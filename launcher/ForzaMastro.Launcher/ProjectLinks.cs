using System.Diagnostics;

namespace ForzaMastro.Launcher;

internal static class ProjectLinks
{
    // Same destination as the supplied Simpsons builder's ProjectLinks.cs.
    internal const string DonationUrl = "https://ko-fi.com/lozazamastro";

    // Preparing this request has no side effects. Only Donate_Click starts it.
    internal static ProcessStartInfo DonationRequest() => new(DonationUrl) { UseShellExecute = true };
}
