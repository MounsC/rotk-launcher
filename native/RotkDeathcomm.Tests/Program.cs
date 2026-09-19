using System.Net;
using System.Net.Sockets;
using System.Net.WebSockets;
using System.Text;
using RotkDeathcomm;
using NAudio.Wave;

static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
string enabled = "[Voice]\nEnable=1\nMicrophoneVolume=50.000000\n";
Check(MicrophonePolicy.Allows(enabled), "enabled microphone");
Check(MicrophonePolicy.Allows(enabled + "ReceiveVolume=0\nGroupVolume=0"), "receive sliders do not authorize or disable microphone");
Check(MicrophonePolicy.InputDevice(enabled) == "Default System Device", "native default device");
Check(MicrophonePolicy.InputDevice(enabled + "[VoiceChat]\nInputDevice=Headset Mic") == "Headset Mic", "respect explicit microphone selection");
Check(MicrophonePolicy.InputDevice(enabled + "[VoiceChat]\nInputDevice=Mic A\nInputDevice=Mic B") == null, "ambiguous device is refused");
foreach (string text in new[] { "", enabled.Replace("Enable=1", "Enable=0"), enabled.Replace("50.000000", "0"),
    enabled.Replace("50.000000", "NaN"), enabled.Replace("50.000000", "Infinity"), enabled.Replace("50.000000", "101"),
    enabled.Replace("50.000000", "-5"), enabled + "Enable=1", enabled.Replace("[Voice]", "[Other]"), "[Voice]\nEnable=1" })
    Check(!MicrophonePolicy.Allows(text), "disabled, missing and malformed mic settings fail closed");

var listener = new TcpListener(IPAddress.Loopback, 0); listener.Start();
using var tcpClient = new TcpClient();
var connecting = tcpClient.ConnectAsync(IPAddress.Loopback, ((IPEndPoint)listener.LocalEndpoint).Port);
using var tcpServer = await listener.AcceptTcpClientAsync(); await connecting; listener.Stop();
using var client = WebSocket.CreateFromStream(tcpClient.GetStream(), false, null, TimeSpan.FromSeconds(10));
using var server = WebSocket.CreateFromStream(tcpServer.GetStream(), true, null, TimeSpan.FromSeconds(10));
var players = new List<FakePlayback>(); var indicator = new FakeIndicator();
bool micAllowed = false;
var microphones = new List<FakeCapture>();
using var session = new DeathcommSession(client, new(1, "unused", "unused", "unused"), indicator,
    microphonePermission: () => Volatile.Read(ref micAllowed),
    createPlayback: until => { var p = new FakePlayback(until); lock (players) players.Add(p); return p; },
    createCapture: () => { var mic = new FakeCapture(); lock (microphones) microphones.Add(mic); return mic; });
using var cancellation = new CancellationTokenSource();
var running = session.Run(cancellation.Token);
const string id = "0123456789abcdef0123456789abcdef";
async Task Send(string text) => await server.SendAsync(Encoding.UTF8.GetBytes(text), WebSocketMessageType.Text, true, CancellationToken.None);
byte[] audio = new byte[656]; Convert.FromHexString(id).CopyTo(audio, 0); audio[16] = 42;
await server.SendAsync(audio, WebSocketMessageType.Binary, true, CancellationToken.None);
await Send($$"""{"type":"capture","id":"{{id}}","remainingMs":4000}""");
await Send($$"""{"type":"listen","id":"{{id}}","remainingMs":300}""");
await server.SendAsync(audio.AsMemory(0, 100), WebSocketMessageType.Binary, false, CancellationToken.None);
await server.SendAsync(audio.AsMemory(100), WebSocketMessageType.Binary, true, CancellationToken.None);
await Task.Delay(100);
lock (players) Check(players.Count == 1 && players[0].Frames == 1, "muted microphone does not block authorized reception; unsolicited audio is ignored");
Check(indicator.Openings == 0, "a capture request cannot activate a disabled microphone");
await Send($$"""{"type":"stop","id":"{{id}}"}""");
await server.SendAsync(audio, WebSocketMessageType.Binary, true, CancellationToken.None);
await Task.Delay(60);
lock (players) Check(players[0].Closed && players[0].Frames == 1, "stop discards playback and late frames");
await Send($$"""{"type":"listen","id":"{{id}}","remainingMs":60}""");
await Task.Delay(160);
lock (players) Check(players.Count == 2 && players[1].Closed, "client deadline closes output without waiting for a server stop");
Volatile.Write(ref micAllowed, true);
await Send($$"""{"type":"capture","id":"{{id}}","remainingMs":4000}""");
await Task.Delay(60);
FakeCapture microphone; lock (microphones) { Check(microphones.Count == 1, "capture starts only after authorization"); microphone = microphones[0]; }
microphone.Push();
using var receiveTimeout = new CancellationTokenSource(1000);
byte[] received = new byte[1000];
var result = await server.ReceiveAsync(received.AsMemory(), receiveTimeout.Token);
Check(result.Count == 656 && result.MessageType == WebSocketMessageType.Binary &&
    received.AsSpan(0, 16).SequenceEqual(Convert.FromHexString(id)) && received[16] == 7, "authorized automatic capture is grant-tagged PCM");
Volatile.Write(ref micAllowed, false); await Task.Delay(70);
Check(microphone.Closed, "changing microphone permission closes capture during the four-second window");
Check(indicator.Openings == 1, "automatic microphone is indicated");
cancellation.Cancel();
try { await running; } catch (Exception e) when (e is OperationCanceledException or WebSocketException) { }
Console.WriteLine("PASS: mic authorization, receive while muted, unsolicited/late audio rejection, stop and deadline cleanup (no physical audio devices used).");

sealed class FakePlayback(long until) : IPlayback
{
    public long Until { get; } = until;
    public int Frames;
    public bool Closed;
    public void Append(byte[] data, int offset, int length) { if (Closed) throw new Exception("closed playback"); Interlocked.Increment(ref Frames); }
    public void Dispose() => Closed = true;
}
sealed class FakeIndicator : ICaptureIndicator
{
    public int Openings;
    public void ShowUntil(long value) { if (value > 0) Interlocked.Increment(ref Openings); }
}
sealed class FakeCapture : ICapture
{
    public event EventHandler<WaveInEventArgs>? DataAvailable;
    public bool Closed;
    public bool Permitted(string settings) => !Closed;
    public void Start() { }
    public void Stop() => Closed = true;
    public void Dispose() => Closed = true;
    public void Push() => DataAvailable?.Invoke(this, new WaveInEventArgs(Enumerable.Repeat((byte)7, 640).ToArray(), 640));
}
