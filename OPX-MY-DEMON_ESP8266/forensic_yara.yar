rule NETHERCAP_Firmware_Strings
{
    meta:
        author = "Memory Forensics Audit"
        description = "Detects NETHERCAP ESP8266 firmware strings in memory dumps"
        version = "1.0"

    strings:
        $s1 = "NETHERCAP"
        $s2 = "OPX-MY-DEMON"
        $s3 = "developer op aminul ff"
        $s4 = "deauther"
        $s5 = "EvilTwin"
        $s6 = "Session Hijack"
        $s7 = "Precise Deauth"
        $s8 = "True Deauth"
        $s9 = "Beacon Spam"
        $s10 = "Rogue AP"

    condition:
        4 of them
}

rule NETHERCAP_Phishing_Pages
{
    meta:
        description = "Detects embedded NETHERCAP phishing HTML in memory"

    strings:
        $fb = "facebook" nocase
        $tenda = "Tenda" nocase
        $pwd_prompt = "Enter WiFi passphrase"
        $conn_lost = "your connection has been terminated"
        $firmware_upgrade = "Firmware Upgrade"
        $internal_err = "500 Internal Server Error"

    condition:
        3 of them
}

rule NETHERCAP_Deauth_Packets
{
    meta:
        description = "Detects NETHERCAP deauth packet pattern in network capture"

    strings:
        $deauth_frame = { C0 00 00 00 FF FF FF FF FF FF }
        $broadcast_bssid = { FF FF FF FF FF FF }

    condition:
        all of them
}

rule ESP8266_Promiscuous_Mode
{
    meta:
        description = "Detects ESP8266 in promiscuous/packet injection mode"

    strings:
        $wifi_promisc = "wifi_promiscuous_enable"
        $pkt_freedom = "wifi_send_pkt_freedom"
        $promisc_cb = "promiscuousCallback"

    condition:
        any of them
}
