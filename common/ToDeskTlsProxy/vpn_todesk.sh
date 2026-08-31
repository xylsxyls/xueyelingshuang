#!/usr/bin/env bash
set -Eeuo pipefail

tcp_tunnel_service_name='vpn-todesk-client.service'
tcp_proxy_service_name='vpn-todesk-proxy.service'
tcp_autostart_service_name='vpn-todesk-autostart.service'
udp_service_name='vpn-todesk-udp-client.service'
udp_autostart_service_name='vpn-todesk-udp-autostart.service'
local_port='52030'
tcp_tunnel_local_port='52031'

usage() {
    cat <<'EOF'
Usage: vpn_todesk.sh start|stop|status|logs [tcp|udp|all]

  start [tcp]   Start TCP mode and enable TCP boot auto-start. This is the default.
  start udp     Start UDP/Hysteria mode and enable UDP boot auto-start.
  stop [tcp]    Stop TCP mode and disable TCP boot auto-start.
  stop udp      Stop UDP mode and disable UDP boot auto-start.
  stop all      Stop both modes and disable all boot auto-start.
  status [mode] Show service state and local listeners.
  logs [mode]   Show recent logs.

ToDesk should always use this local proxy endpoint:
  127.0.0.1:52030

Only one Ubuntu client mode can run at a time because both modes listen on
127.0.0.1:52030. If no mode is provided, tcp is used.
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

tcp_units_exist() {
    systemctl cat "$tcp_tunnel_service_name" >/dev/null 2>&1 \
        && systemctl cat "$tcp_proxy_service_name" >/dev/null 2>&1 \
        && systemctl cat "$tcp_autostart_service_name" >/dev/null 2>&1
}

udp_units_exist() {
    systemctl cat "$udp_service_name" >/dev/null 2>&1 \
        && systemctl cat "$udp_autostart_service_name" >/dev/null 2>&1
}

require_tcp_units() {
    tcp_units_exist || die 'TCP client is not initialized; run ./vpn_todesk_init.sh first'
}

require_udp_units() {
    udp_units_exist || die 'UDP client is not initialized; rerun ./vpn_todesk_init.sh with the new Hysteria files'
}

local_listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)"
}

tcp_tunnel_listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${tcp_tunnel_local_port}([[:space:]]|$)"
}

show_tcp_failure() {
    run_root systemctl status "$tcp_autostart_service_name" "$tcp_tunnel_service_name" "$tcp_proxy_service_name" --no-pager || true
    run_root journalctl -u "$tcp_autostart_service_name" -u "$tcp_tunnel_service_name" -u "$tcp_proxy_service_name" -n 180 --no-pager || true
}

show_udp_failure() {
    run_root systemctl status "$udp_autostart_service_name" "$udp_service_name" --no-pager || true
    run_root journalctl -u "$udp_autostart_service_name" -u "$udp_service_name" -n 180 --no-pager || true
}

disable_units_quietly() {
    run_root systemctl disable "$@" >/dev/null 2>&1 || true
}

stop_units_quietly() {
    run_root systemctl stop "$@" >/dev/null 2>&1 || true
    run_root systemctl reset-failed "$@" >/dev/null 2>&1 || true
}

start_tcp() {
    require_tcp_units

    printf 'Stopping UDP mode first, because both modes use 127.0.0.1:%s...\n' "$local_port"
    stop_units_quietly "$udp_autostart_service_name" "$udp_service_name"
    disable_units_quietly "$udp_autostart_service_name" "$udp_service_name"

    printf 'Stopping the previous managed TCP client, if any...\n'
    if ! run_root systemctl stop "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name"; then
        show_tcp_failure
        die 'could not stop the previous managed TCP client'
    fi
    run_root systemctl reset-failed "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name" >/dev/null 2>&1 || true
    disable_units_quietly "$tcp_proxy_service_name" "$tcp_tunnel_service_name"

    printf 'Starting a fresh TCP client through the same manager used at boot...\n'
    if ! run_root systemctl start "$tcp_autostart_service_name"; then
        show_tcp_failure
        die 'could not start the managed TCP client'
    fi

    local attempt
    for attempt in {1..20}; do
        if systemctl is-active --quiet "$tcp_tunnel_service_name" && tcp_tunnel_listener_is_ready; then
            break
        fi
        sleep 0.25
    done
    if ! tcp_tunnel_listener_is_ready; then
        show_tcp_failure
        die "the TCP tunnel did not listen on 127.0.0.1:${tcp_tunnel_local_port}"
    fi

    printf 'Verifying the local ToDesk proxy adapter...\n'
    for attempt in {1..20}; do
        if systemctl is-active --quiet "$tcp_proxy_service_name" && local_listener_is_ready; then
            printf 'Local TCP encrypted-proxy client is ready.\n'
            printf 'ToDesk proxy endpoint: 127.0.0.1:%s\n' "$local_port"
            printf 'Leave the ToDesk proxy username and password empty.\n'
            printf 'Enabling boot auto-start for TCP mode...\n'
            if ! run_root systemctl enable "$tcp_autostart_service_name" >/dev/null; then
                show_tcp_failure
                die 'could not enable boot auto-start for TCP mode'
            fi
            printf 'TCP boot auto-start is enabled. Run ./vpn_todesk.sh stop tcp to disable it.\n'
            return 0
        fi
        sleep 0.25
    done

    printf 'The TCP client did not become ready.\n' >&2
    show_tcp_failure
    return 1
}

start_udp() {
    require_udp_units

    printf 'Stopping TCP mode first, because both modes use 127.0.0.1:%s...\n' "$local_port"
    stop_units_quietly "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name"
    disable_units_quietly "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name"

    printf 'Stopping the previous managed UDP client, if any...\n'
    if ! run_root systemctl stop "$udp_autostart_service_name" "$udp_service_name"; then
        show_udp_failure
        die 'could not stop the previous managed UDP client'
    fi
    run_root systemctl reset-failed "$udp_autostart_service_name" "$udp_service_name" >/dev/null 2>&1 || true

    printf 'Starting a fresh UDP/Hysteria client through the same manager used at boot...\n'
    if ! run_root systemctl start "$udp_autostart_service_name"; then
        show_udp_failure
        die 'could not start the managed UDP client'
    fi

    local attempt
    for attempt in {1..20}; do
        if systemctl is-active --quiet "$udp_service_name" && local_listener_is_ready; then
            printf 'Local UDP/Hysteria encrypted-proxy client is ready.\n'
            printf 'ToDesk proxy endpoint: 127.0.0.1:%s\n' "$local_port"
            printf 'Leave the ToDesk proxy username and password empty.\n'
            printf 'Enabling boot auto-start for UDP mode...\n'
            if ! run_root systemctl enable "$udp_autostart_service_name" >/dev/null; then
                show_udp_failure
                die 'could not enable boot auto-start for UDP mode'
            fi
            printf 'UDP boot auto-start is enabled. Run ./vpn_todesk.sh stop udp to disable it.\n'
            return 0
        fi
        sleep 0.25
    done

    printf 'The UDP client did not become ready.\n' >&2
    show_udp_failure
    return 1
}

stop_tcp() {
    require_tcp_units

    if ! run_root systemctl stop "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name"; then
        show_tcp_failure
        die 'could not stop the managed TCP client'
    fi
    run_root systemctl reset-failed "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name" >/dev/null 2>&1 || true

    printf 'Disabling boot auto-start for TCP mode...\n'
    if ! run_root systemctl disable "$tcp_autostart_service_name" "$tcp_proxy_service_name" "$tcp_tunnel_service_name" >/dev/null; then
        show_tcp_failure
        die 'could not disable boot auto-start for TCP mode'
    fi

    printf 'TCP client stopped and TCP boot auto-start is disabled.\n'
}

stop_udp() {
    require_udp_units

    if ! run_root systemctl stop "$udp_autostart_service_name" "$udp_service_name"; then
        show_udp_failure
        die 'could not stop the managed UDP client'
    fi
    run_root systemctl reset-failed "$udp_autostart_service_name" "$udp_service_name" >/dev/null 2>&1 || true

    printf 'Disabling boot auto-start for UDP mode...\n'
    if ! run_root systemctl disable "$udp_autostart_service_name" "$udp_service_name" >/dev/null; then
        show_udp_failure
        die 'could not disable boot auto-start for UDP mode'
    fi

    printf 'UDP client stopped and UDP boot auto-start is disabled.\n'
}

stop_all() {
    tcp_units_exist && stop_tcp || true
    udp_units_exist && stop_udp || true
    if local_listener_is_ready; then
        printf 'Warning: another process still owns 127.0.0.1:%s; it was not killed.\n' "$local_port" >&2
        return 1
    fi
}

show_tcp_status() {
    require_tcp_units
    run_root systemctl status "$tcp_autostart_service_name" "$tcp_tunnel_service_name" "$tcp_proxy_service_name" --no-pager || true
    printf '\nTCP boot auto-start state:\n'
    systemctl is-enabled "$tcp_autostart_service_name" "$tcp_tunnel_service_name" "$tcp_proxy_service_name" 2>/dev/null || true
    printf '\nLocal ToDesk proxy listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)" || true
    printf '\nInternal stunnel SOCKS listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${tcp_tunnel_local_port}([[:space:]]|$)" || true
}

show_udp_status() {
    require_udp_units
    run_root systemctl status "$udp_autostart_service_name" "$udp_service_name" --no-pager || true
    printf '\nUDP boot auto-start state:\n'
    systemctl is-enabled "$udp_autostart_service_name" "$udp_service_name" 2>/dev/null || true
    printf '\nLocal ToDesk proxy listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)" || true
}

show_logs_tcp() {
    require_tcp_units
    run_root journalctl -u "$tcp_autostart_service_name" -u "$tcp_tunnel_service_name" -u "$tcp_proxy_service_name" -n 220 --no-pager
}

show_logs_udp() {
    require_udp_units
    run_root journalctl -u "$udp_autostart_service_name" -u "$udp_service_name" -n 220 --no-pager
}

action=${1:-}
mode=${2:-tcp}
if (($# > 2)); then
    usage >&2
    exit 2
fi

case $mode in
    tcp|udp|all)
        ;;
    *)
        die "unsupported mode: $mode"
        ;;
esac

case $action in
    start)
        if [[ $mode == all ]]; then
            die 'Ubuntu side cannot start tcp and udp at the same time; choose tcp or udp'
        elif [[ $mode == udp ]]; then
            start_udp
        else
            start_tcp
        fi
        ;;
    stop)
        if [[ $mode == all ]]; then
            stop_all
        elif [[ $mode == udp ]]; then
            stop_udp
        else
            stop_tcp
        fi
        ;;
    status)
        if [[ $mode == all ]]; then
            show_tcp_status || true
            printf '\n'
            show_udp_status || true
        elif [[ $mode == udp ]]; then
            show_udp_status
        else
            show_tcp_status
        fi
        ;;
    logs)
        if [[ $mode == all ]]; then
            show_logs_tcp || true
            printf '\n'
            show_logs_udp || true
        elif [[ $mode == udp ]]; then
            show_logs_udp
        else
            show_logs_tcp
        fi
        ;;
    -h|--help|'')
        usage
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac
