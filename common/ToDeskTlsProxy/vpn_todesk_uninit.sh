#!/usr/bin/env bash
set -Eeuo pipefail

tunnel_service_name='vpn-todesk-client.service'
proxy_service_name='vpn-todesk-proxy.service'
client_install_dir='/etc/vpn-todesk'
tunnel_unit_path="/etc/systemd/system/$tunnel_service_name"
proxy_unit_path="/etc/systemd/system/$proxy_service_name"

usage() {
    cat <<'EOF'
Usage: ./vpn_todesk_uninit.sh

Stop and uninstall the ToDesk VPN client services and local VPN files.
Ubuntu packages installed by vpn_todesk_init.sh are intentionally kept.
EOF
}

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

run_root() {
    if ((EUID == 0)); then
        "$@"
    else
        sudo "$@"
    fi
}

stop_service_if_present() {
    local service_name=$1

    if ! systemctl list-unit-files "$service_name" >/dev/null 2>&1 \
        && ! systemctl status "$service_name" >/dev/null 2>&1; then
        printf 'Service %s is not installed; skipping stop.\n' "$service_name"
        return 0
    fi

    if systemctl is-active --quiet "$service_name"; then
        printf 'Stopping %s...\n' "$service_name"
    else
        printf 'Service %s is already stopped; continuing.\n' "$service_name"
    fi

    run_root systemctl stop "$service_name" >/dev/null 2>&1 || true
    run_root systemctl reset-failed "$service_name" >/dev/null 2>&1 || true
}

case ${1:-} in
    '')
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

[[ -r /etc/os-release ]] || die 'cannot identify this Linux distribution'
command -v sudo >/dev/null 2>&1 || ((EUID == 0)) \
    || die 'sudo is required when running as a normal user'
command -v systemctl >/dev/null 2>&1 || die 'systemd is required'

if ((EUID != 0)); then
    sudo -v
fi

printf 'Uninstalling the ToDesk VPN client...\n'
stop_service_if_present "$proxy_service_name"
stop_service_if_present "$tunnel_service_name"

if systemctl is-active --quiet "$proxy_service_name" \
    || systemctl is-active --quiet "$tunnel_service_name"; then
    run_root systemctl status "$proxy_service_name" "$tunnel_service_name" --no-pager || true
    die 'one of the ToDesk VPN client services is still running'
fi

printf 'Disabling and removing systemd units...\n'
run_root systemctl disable "$proxy_service_name" "$tunnel_service_name" >/dev/null 2>&1 || true
run_root rm -f -- "$proxy_unit_path" "$tunnel_unit_path"
run_root systemctl daemon-reload
run_root systemctl reset-failed "$proxy_service_name" "$tunnel_service_name" >/dev/null 2>&1 || true

case $client_install_dir in
    /etc/vpn-todesk)
        printf 'Removing %s...\n' "$client_install_dir"
        run_root rm -rf -- "$client_install_dir"
        ;;
    *)
        die "refusing to remove unexpected install directory: $client_install_dir"
        ;;
esac

printf '\nUbuntu client uninstall completed.\n'
printf 'Kept Ubuntu packages such as stunnel4/python3/openssl.\n'
printf 'Kept this project directory, pki-out/, and windows-server-package/ if present.\n'
