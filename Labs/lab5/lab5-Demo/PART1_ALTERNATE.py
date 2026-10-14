import platform
import time
import subprocess
import re
import numpy as np
import pandas as pd
import plotly.express as px


def _run(cmd: str) -> str:
    """Run a shell command and return stdout+stderr as text (or raise with context)."""
    try:
        p = subprocess.run(
            cmd,
            shell=True,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        return p.stdout
    except subprocess.CalledProcessError as e:
        out = e.stdout if isinstance(e.stdout, str) else ""
        raise RuntimeError(f"Command failed: {cmd}\nOutput:\n{out}") from e


def _linux_wifi_iface() -> str | None:
    """Best-effort detection of the active Wi-Fi interface on Linux."""
    # Try NetworkManager (common on Ubuntu)
    try:
        out = _run("nmcli -t -f DEVICE,TYPE,STATE dev status")
        wifi_any = None
        for line in out.splitlines():
            parts = line.split(":")
            if len(parts) < 3:
                continue
            dev, typ, state = parts[0], parts[1], parts[2]
            if typ == "wifi":
                wifi_any = dev
                if state == "connected":
                    return dev
        if wifi_any:
            return wifi_any
    except Exception:
        pass

    # Fallback to iw
    try:
        out = _run('iw dev | awk \'$1=="Interface"{print $2; exit}\'')
        iface = out.strip()
        return iface if iface else None
    except Exception:
        return None


def get_wifi_signal_strength(iface: str | None = None) -> int:
    """
    Get Wi-Fi signal strength in dBm (negative integer, e.g., -55).
    Works on Linux/Windows/macOS with best-effort parsing.

    On Linux, prefers: `iw dev <iface> link` (more consistent than iwconfig).
    """
    osname = platform.system()

    if osname == "Linux":
        iface = iface or _linux_wifi_iface()

        # Prefer iw dev <iface> link
        if iface:
            txt = _run(f"iw dev {iface} link")
            m = re.search(r"signal:\s*(-?\d+)\s*dBm", txt)
            if m:
                return int(m.group(1))

        # Fallback to iwconfig (older but sometimes present)
        cmd = f"iwconfig {iface}" if iface else "iwconfig"
        txt = _run(cmd)

        # Handle multiple formats:
        #   Signal level=-55 dBm
        #   Signal level=-55 dBm  (with variable spacing)
        #   Signal level:-55 dBm
        m = re.search(r"Signal level[=:-]\s*(-?\d+)\s*dBm", txt)
        if not m:
            raise RuntimeError(
                "Couldn't parse Wi-Fi signal level on Linux.\n"
                f"Tried iface={iface!r}\n"
                f"Last command: {cmd}\n"
                f"Output:\n{txt}\n\n"
                "Tip: your Wi-Fi interface may not be wlan0. "
                "Run `nmcli dev status` or `iw dev` to see the interface name."
            )
        return int(m.group(1))

    if osname == "Windows":
        txt = _run("netsh wlan show interfaces")
        m = re.search(r"Signal\s*:\s*(\d+)%", txt)
        if not m:
            raise RuntimeError(
                "Couldn't parse Wi-Fi signal quality on Windows.\n"
                f"Output:\n{txt}"
            )
        q = int(m.group(1))  # 0..100
        # Common rough conversion used in many scripts
        return int(round(-100 + q / 2))

    if osname == "Darwin":
        # Newer macOS
        txt = _run("wdutil info")
        m = re.search(r"\bRSSI\s*:\s*(-?\d+)", txt)
        if m:
            return int(m.group(1))

        # Fallback (older/private tool)
        txt2 = _run(
            "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport -I"
        )
        m2 = re.search(r"\bagrCtlRSSI:\s*(-?\d+)", txt2)
        if not m2:
            raise RuntimeError(
                "Couldn't parse Wi-Fi RSSI on macOS.\n"
                f"wdutil output:\n{txt}\n\n"
                f"airport output:\n{txt2}"
            )
        return int(m2.group(1))

    raise RuntimeError(f"Unsupported OS: {osname}")


def main():
    # Choose at least 5 locations to sample the signal strength at
    locations = ["bedroom", "living room", "kitchen", "bathroom", "garage"]
    samples_per_location = 10
    time_between_samples = 1  # seconds

    data = []
    for location in locations:
        print(f"\nGo to the {location} and press Enter to start sampling...")
        input()

        signal_strengths = []
        for i in range(samples_per_location):
            try:
                s = get_wifi_signal_strength()
                signal_strengths.append(s)
                print(f"[{location}] sample {i+1}/{samples_per_location}: {s} dBm")
            except Exception as e:
                print(f"[{location}] sample {i+1}/{samples_per_location}: ERROR -> {e}")
            time.sleep(time_between_samples)

        if len(signal_strengths) == 0:
            mean_dbm = float("nan")
            std_dbm = float("nan")
        else:
            mean_dbm = float(np.mean(signal_strengths))
            std_dbm = float(np.std(signal_strengths, ddof=1)) if len(signal_strengths) > 1 else 0.0

        data.append((location, mean_dbm, std_dbm))

    df = pd.DataFrame(data, columns=["location", "signal_strength_mean", "signal_strength_std"])
    print("\nResults:")
    print(df)

    fig = px.bar(
        df,
        x="location",
        y="signal_strength_mean",
        error_y="signal_strength_std",
        title="Wi-Fi Signal Strength by Location (mean ± 1 std)",
        labels={"signal_strength_mean": "Signal Strength (dBm)", "location": "Location"},
    )

    # Write image (requires kaleido). If not available, write HTML instead.
    try:
        fig.write_image("signal_strength.png")
        print("\nSaved plot: signal_strength.png")
    except Exception as e:
        fig.write_html("signal_strength.html")
        print("\nCould not write PNG (likely missing kaleido). Saved: signal_strength.html")
        print(f"PNG error was: {e}")


if __name__ == "__main__":
    main()
