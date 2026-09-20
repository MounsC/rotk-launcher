namespace RotkDeathcomm;

internal static class VoicePolicy
{
    // Both native switches must permit automatic microphone capture. Missing
    // or duplicate settings cannot authorize capture; reception is independent.
    public static bool ChatEnabled(string text) =>
        ReadSetting(text, "Voice", "Enable") == "1" &&
        ReadSetting(text, "VoiceChat", "ProximityEnabled") == "1";

    internal static string? ReadSetting(string text, string section, string key)
    {
        bool selected = false; string? value = null;
        foreach (string raw in text.Split('\n'))
        {
            string line = raw.Trim();
            if (line.StartsWith(';') || line.StartsWith('#')) continue;
            if (line.StartsWith('[')) { selected = line.Equals($"[{section}]", StringComparison.OrdinalIgnoreCase); continue; }
            if (!selected) continue;
            var parts = line.Split('=', 2);
            if (parts.Length != 2 || !parts[0].Trim().Equals(key, StringComparison.OrdinalIgnoreCase)) continue;
            if (value != null) return null;
            value = parts[1].Trim();
        }
        return value;
    }
}
