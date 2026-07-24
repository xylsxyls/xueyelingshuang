#!/usr/bin/env bash
set -Eeuo pipefail

tunnel_service_name='vpn-todesk-client.service'
proxy_service_name='vpn-todesk-proxy.service'
local_port='52030'
tunnel_local_port='52031'

usage() {
    cat <<'EOF'
Usage: vpn_todesk.sh start|stop|status|logs

  start   Stop the previously managed client, then start a fresh client.
  stop    Stop the managed client process.
  status  Show service state and the local proxy listener.
  logs    Show the latest client logs.
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
        && systemctl cat "$proxy_service_name" >/dev/null 2>&1
}

listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)"
}

tunnel_listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:${tunnel_local_port}([[:space:]]|$)"
}

show_failure() {
    run_root systemctl status "$tunnel_service_name" "$proxy_service_name" --no-pager || true
    run_root journalctl -u "$tunnel_service_name" -u "$proxy_service_name" -n 120 --no-pager || true
}

start_client() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'

    printf 'Stopping the previous managed client, if any...\n'
    if ! run_root systemctl stop "$proxy_service_name" "$tunnel_service_name"; then
        show_failure
        die 'could not stop the previous managed client'
    fi
    run_root systemctl reset-failed "$proxy_service_name" "$tunnel_service_name" >/dev/null 2>&1 || true

    printf 'Starting a fresh encrypted tunnel...\n'
    if ! run_root systemctl start "$tunnel_service_name"; then
        show_failure
        die 'could not start the managed tunnel'
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

    printf 'Starting the local ToDesk proxy adapter...\n'
    if ! run_root systemctl start "$proxy_service_name"; then
        show_failure
        die 'could not start the local proxy adapter'
    fi

    for attempt in {1..20}; do
        if systemctl is-active --quiet "$proxy_service_name" && listener_is_ready; then
            printf 'Local encrypted-proxy client is ready.\n'
            printf 'ToDesk proxy endpoint: 127.0.0.1:%s\n' "$local_port"
            printf 'Leave the ToDesk proxy username and password empty.\n'
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
    if ! run_root systemctl stop "$proxy_service_name" "$tunnel_service_name"; then
        show_failure
        die 'could not stop the managed client'
    fi

    if systemctl is-active --quiet "$proxy_service_name" || systemctl is-active --quiet "$tunnel_service_name"; then
        show_failure
        die 'the managed client is still active'
    fi

    printf 'VPN client stopped.\n'
    if listener_is_ready; then
        printf 'Warning: another process still owns 127.0.0.1:%s; it was not killed.\n' \
            "$local_port" >&2
    fi
}

show_status() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'
    run_root systemctl status "$tunnel_service_name" "$proxy_service_name" --no-pager || true
    printf '\nLocal ToDesk proxy listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${local_port}([[:space:]]|$)" || true
    printf '\nInternal stunnel SOCKS listener:\n'
    ss -ltnp 2>/dev/null | grep -E "127\\.0\\.0\\.1:${tunnel_local_port}([[:space:]]|$)" || true
}

show_logs() {
    unit_exists || die 'client is not initialized; run ./vpn_todesk_init.sh first'
    run_root journalctl -u "$tunnel_service_name" -u "$proxy_service_name" -n 160 --no-pager
}

case ${1:-} in
    start)
        start_client
        ;;
    stop)
        stop_client
        ;;
    status)
        show_status
        ;;
    logs)
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
