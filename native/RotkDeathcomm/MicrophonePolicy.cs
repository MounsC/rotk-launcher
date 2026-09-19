using System.Globalization;

namespace RotkDeathcomm;

internal static class MicrophonePolicy
{
    public static string? InputDevice(string text)
    {
        bool voice = false; string? device = null;
        foreach (string raw in text.Split('\n'))
        {
            string line = raw.Trim();
            if (line.StartsWith('[')) { voice = line.Equals("[VoiceChat]", StringComparison.OrdinalIgnoreCase); continue; }
            if (!voice) continue;
            var parts = line.Split('=', 2);
            if (parts.Length != 2 || !parts[0].Trim().Equals("InputDevice", StringComparison.OrdinalIgnoreCase)) continue;
            if (device != null) return null;
            device = parts[1].Trim();
        }
        return device == "" ? null : device ?? "Default System Device";
    }
    // Missing, ambiguous or unreadable preferences never authorize automatic capture.
    public static bool Allows(string text)
    {
        bool voice = false;
        string? enabled = null, volume = null;
        foreach (string raw in text.Split('\n'))
        {
            string line = raw.Trim();
            if (line.StartsWith(';') || line.StartsWith('#')) continue;
            if (line.StartsWith('[')) { voice = line.Equals("[Voice]", StringComparison.OrdinalIgnoreCase); continue; }
            if (!voice) continue;
            var parts = line.Split('=', 2);
            if (parts.Length != 2) continue;
            string key = parts[0].Trim(), value = parts[1].Trim();
            if (key.Equals("Enable", StringComparison.OrdinalIgnoreCase))
            { if (enabled != null) return false; enabled = value; }
            if (key.Equals("MicrophoneVolume", StringComparison.OrdinalIgnoreCase))
            { if (volume != null) return false; volume = value; }
        }
        return enabled == "1" && double.TryParse(volume, NumberStyles.Float, CultureInfo.InvariantCulture, out double level)
            && double.IsFinite(level) && level > 0 && level <= 100;
    }
}
