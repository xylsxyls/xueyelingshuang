#!/usr/bin/env bash
set -Eeuo pipefail

tunnel_service_name='vpn-todesk-client.service'
proxy_service_name='vpn-todesk-proxy.service'
autostart_service_name='vpn-todesk-autostart.service'
local_port='52030'
tunnel_local_port='52031'

usage() {
    cat <<'EOF'
Usage: vpn_todesk.sh start|stop|status|logs [tcp]

  start [tcp]   Stop the previous TCP client, start a fresh one, and enable boot auto-start.
  stop [tcp]    Stop the TCP client and disable boot auto-start.
  status [tcp]  Show service state, auto-start state, and the local proxy listener.
  logs [tcp]    Show the latest TCP client logs.

The mode argument is optional. The default mode is tcp.
UDP mode is reserved for a future implementation.
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

unit_exists() {
    systemctl cat "$tunnel_service_name" >/dev/null 2>&1 \
        && systemctl cat "$proxy_service_name" >/dev/null 2>&1 \
        && systemctl cat "$autostart_service_name" >/dev/null 2>&1
}

listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)"
}

tunnel_listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${tunnel_local_port}([[:space:]]|$)"
}

show_failure() {
    run_root systemctl status "$autostart_service_name" "$tunnel_service_name" "$proxy_service_name" --no-pager || true
    run_root journalctl -u "$autostart_service_name" -u "$tunnel_service_name" -u "$proxy_service_name" -n 160 --no-pager || true
}

require_mode() {
    local mode=${1:-tcp}

    case $mode in
        tcp)
            return 0
            ;;
        udp)
            die 'UDP mode is not implemented yet; use tcp for now'
            ;;
        *)
            die "unsupported mode: $mode"
            ;;
    esac
}

start_client() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'

    printf 'Stopping the previous managed client, if any...\n'
    if ! run_root systemctl stop "$autostart_service_name" "$proxy_service_name" "$tunnel_service_name"; then
        show_failure
        die 'could not stop the previous managed client'
    fi
    run_root systemctl reset-failed "$autostart_service_name" "$proxy_service_name" "$tunnel_service_name" >/dev/null 2>&1 || true
    run_root systemctl disable "$proxy_service_name" "$tunnel_service_name" >/dev/null 2>&1 || true

    printf 'Starting a fresh TCP client through the same manager used at boot...\n'
    if ! run_root systemctl start "$autostart_service_name"; then
        show_failure
        die 'could not start the managed TCP client'
    fi

    local attempt
    for attempt in {1..20}; do
        if systemctl is-active --quiet "$tunnel_service_name" && tunnel_listener_is_ready; then
            break
        fi
        sleep 0.25
    done
    if ! tunnel_listener_is_ready; then
        show_failure
        die "the encrypted tunnel did not listen on 127.0.0.1:${tunnel_local_port}"
    fi

    printf 'Verifying the local ToDesk proxy adapter...\n'
    for attempt in {1..20}; do
        if systemctl is-active --quiet "$proxy_service_name" && listener_is_ready; then
            printf 'Local encrypted-proxy client is ready.\n'
            printf 'ToDesk proxy endpoint: 127.0.0.1:%s\n' "$local_port"
            printf 'Leave the ToDesk proxy username and password empty.\n'
            printf 'Enabling boot auto-start for the TCP client...\n'
            if ! run_root systemctl enable "$autostart_service_name" >/dev/null; then
                show_failure
                die 'could not enable boot auto-start for the TCP client'
            fi
            printf 'Boot auto-start is enabled. Run ./vpn_todesk.sh stop to disable it.\n'
            return 0
        fi
        sleep 0.25
    done

    printf 'The client did not become ready.\n' >&2
    show_failure
    return 1
}

stop_client() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'
    if ! run_root systemctl stop "$autostart_service_name" "$proxy_service_name" "$tunnel_service_name"; then
        show_failure
        die 'could not stop the managed client'
    fi

    if systemctl is-active --quiet "$autostart_service_name" \
        || systemctl is-active --quiet "$proxy_service_name" \
        || systemctl is-active --quiet "$tunnel_service_name"; then
        show_failure
        die 'the managed client is still active'
    fi

    printf 'Disabling boot auto-start for the TCP client...\n'
    if ! run_root systemctl disable "$autostart_service_name" "$proxy_service_name" "$tunnel_service_name" >/dev/null; then
        show_failure
        die 'could not disable boot auto-start for the TCP client'
    fi

    printf 'VPN client stopped and boot auto-start is disabled.\n'
    if listener_is_ready; then
        printf 'Warning: another process still owns 127.0.0.1:%s; it was not killed.\n' \
            "$local_port" >&2
    fi
}

show_status() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'
    run_root systemctl status "$autostart_service_name" "$tunnel_service_name" "$proxy_service_name" --no-pager || true
    printf '\nBoot auto-start state:\n'
    systemctl is-enabled "$autostart_service_name" "$tunnel_service_name" "$proxy_service_name" 2>/dev/null || true
    printf '\nLocal ToDesk proxy listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)" || true
    printf '\nInternal stunnel SOCKS listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${tunnel_local_port}([[:space:]]|$)" || true
}

show_logs() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'
    run_root journalctl -u "$autostart_service_name" -u "$tunnel_service_name" -u "$proxy_service_name" -n 220 --no-pager
}

action=${1:-}
mode=${2:-tcp}
if (($# > 2)); then
    usage >&2
    exit 2
fi

case $action in
    start)
        require_mode "$mode"
        start_client
        ;;
    stop)
        require_mode "$mode"
        stop_client
        ;;
    status)
        require_mode "$mode"
        show_status
        ;;
    logs)
        require_mode "$mode"
        show_logs
        ;;
    -h|--help|'')
        usage
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac
