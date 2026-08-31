#!/usr/bin/env bash
set -Eeuo pipefail
umask 077

server_ip='175.24.198.162'
server_port='52030'
local_port='52030'
tunnel_local_port='52031'
client_name='ubuntu-office-01'
service_name='vpn-todesk-client.service'
proxy_service_name='vpn-todesk-proxy.service'
todesk_autostart_service_name='vpn-todesk-autostart.service'
todesk_autostart_script_name='vpn-todesk-start-tcp.sh'
udp_service_name='vpn-todesk-udp-client.service'
udp_autostart_service_name='vpn-todesk-udp-autostart.service'
udp_autostart_script_name='vpn-todesk-start-udp.sh'
hysteria_linux_binary_name='hysteria-linux-amd64'
hysteria_windows_binary_name='hysteria-windows-amd64.exe'
nssm_binary_name='nssm.exe'
service_user='vpn-todesk'

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

usage() {
    cat <<'EOF'
Usage: ./vpn_todesk_init.sh [--client-only]

  --client-only  Reinstall only the Ubuntu client and local proxy adapter.
                 Use this after the Windows server has already been deployed.
EOF
}

client_only=0
case ${1:-} in
    '')
        ;;
    --client-only)
        client_only=1
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

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
pki_dir="$script_dir/pki-out"
windows_package="$script_dir/windows-server-package"
server_init_script="$script_dir/vpn_todesk_server_init.bat"
server_control_script="$script_dir/vpn_todesk_server.bat"
server_uninit_script="$script_dir/vpn_todesk_server_uninit.bat"
proxy_adapter_script="$script_dir/todesk_proxy_adapter.py"
hysteria_linux_binary="$script_dir/$hysteria_linux_binary_name"
hysteria_windows_binary="$script_dir/$hysteria_windows_binary_name"
nssm_binary="$script_dir/$nssm_binary_name"
client_install_dir='/etc/vpn-todesk'
unit_path="/etc/systemd/system/$service_name"
proxy_unit_path="/etc/systemd/system/$proxy_service_name"
todesk_autostart_unit_path="/etc/systemd/system/$todesk_autostart_service_name"
udp_unit_path="/etc/systemd/system/$udp_service_name"
udp_autostart_unit_path="/etc/systemd/system/$udp_autostart_service_name"

temporary_dirs=()
cleanup() {
    local path
    for path in "${temporary_dirs[@]}"; do
        [[ -n $path && -e $path ]] || continue
        case $path in
            "$script_dir"/.vpn-todesk-*) rm -rf -- "$path" ;;
        esac
    done
}
trap cleanup EXIT

generate_private_pki() {
    local output_dir=$1
    local offline_dir="$output_dir/offline-ca"
    local work_dir="$offline_dir/work"
    local server_dir="$output_dir/windows-server"
    local client_dir="$output_dir/ubuntu-client"

    [[ ! -e $output_dir ]] || die "PKI output already exists: $output_dir"
    mkdir -p "$work_dir" "$server_dir" "$client_dir"

    cat >"$work_dir/ca.ext" <<'EOF'
basicConstraints = critical, CA:TRUE, pathlen:0
keyUsage = critical, keyCertSign, cRLSign
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always
EOF

    cat >"$work_dir/server.ext" <<EOF
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
subjectAltName = IP:$server_ip
EOF

    cat >"$work_dir/client.ext" <<EOF
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
subjectAltName = DNS:$client_name
EOF

    printf 'Generating the private server CA...\n'
    openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:4096 \
        -out "$offline_dir/server-ca.key"
    openssl req -new -sha256 -key "$offline_dir/server-ca.key" \
        -subj '/CN=ToDesk TLS Proxy Server CA' -out "$work_dir/server-ca.csr"
    openssl x509 -req -sha256 -days 3650 -in "$work_dir/server-ca.csr" \
        -signkey "$offline_dir/server-ca.key" -extfile "$work_dir/ca.ext" \
        -out "$offline_dir/server-ca.crt"

    printf 'Generating the private client CA...\n'
    openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:4096 \
        -out "$offline_dir/client-ca.key"
    openssl req -new -sha256 -key "$offline_dir/client-ca.key" \
        -subj '/CN=ToDesk TLS Proxy Client CA' -out "$work_dir/client-ca.csr"
    openssl x509 -req -sha256 -days 3650 -in "$work_dir/client-ca.csr" \
        -signkey "$offline_dir/client-ca.key" -extfile "$work_dir/ca.ext" \
        -out "$offline_dir/client-ca.crt"

    printf 'Generating the Windows server certificate...\n'
    openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 \
        -out "$server_dir/server.key"
    openssl req -new -sha256 -key "$server_dir/server.key" \
        -subj "/CN=$server_ip" -out "$work_dir/server.csr"
    openssl x509 -req -sha256 -days 825 -in "$work_dir/server.csr" \
        -CA "$offline_dir/server-ca.crt" -CAkey "$offline_dir/server-ca.key" \
        -CAserial "$offline_dir/server-ca.srl" -CAcreateserial \
        -extfile "$work_dir/server.ext" -out "$server_dir/server.crt"
    cat "$server_dir/server.crt" "$offline_dir/server-ca.crt" \
        >"$server_dir/server-fullchain.pem"
    cp "$offline_dir/client-ca.crt" "$server_dir/client-ca.crt"

    printf 'Generating the Ubuntu client certificate...\n'
    openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:3072 \
        -out "$client_dir/client.key"
    openssl req -new -sha256 -key "$client_dir/client.key" \
        -subj "/CN=$client_name" -out "$work_dir/client.csr"
    openssl x509 -req -sha256 -days 825 -in "$work_dir/client.csr" \
        -CA "$offline_dir/client-ca.crt" -CAkey "$offline_dir/client-ca.key" \
        -CAserial "$offline_dir/client-ca.srl" -CAcreateserial \
        -extfile "$work_dir/client.ext" -out "$client_dir/client.crt"
    cat "$client_dir/client.crt" "$offline_dir/client-ca.crt" \
        >"$client_dir/client-fullchain.pem"
    cp "$offline_dir/server-ca.crt" "$client_dir/server-ca.crt"

    chmod 600 "$offline_dir"/*.key "$server_dir/server.key" "$client_dir/client.key"
    chmod 644 "$offline_dir"/*.crt "$server_dir"/*.crt "$server_dir"/*.pem \
        "$client_dir"/*.crt "$client_dir"/*.pem

    openssl verify -purpose sslserver -CAfile "$offline_dir/server-ca.crt" \
        "$server_dir/server.crt" >/dev/null
    openssl verify -purpose sslclient -CAfile "$offline_dir/client-ca.crt" \
        "$client_dir/client.crt" >/dev/null
    openssl x509 -in "$server_dir/server.crt" -noout -checkip "$server_ip" \
        >/dev/null

    cat >"$output_dir/README.txt" <<EOF
Server identity: $server_ip
Client identity: $client_name

The endpoint files are inputs for vpn_todesk_init.sh.
Deploy only the generated windows-server-package directory.
Keep hysteria-auth.txt private; it is the UDP tunnel password.
Keep offline-ca in encrypted offline storage; never deploy or commit it.
EOF
}

[[ -r /etc/os-release ]] || die 'cannot identify this Linux distribution'
. /etc/os-release
[[ ${ID:-} == 'ubuntu' || ${ID_LIKE:-} == *ubuntu* || ${ID_LIKE:-} == *debian* ]] \
    || die 'this initializer supports Ubuntu/Debian systems only'

command -v sudo >/dev/null 2>&1 || ((EUID == 0)) \
    || die 'sudo is required when running as a normal user'
command -v systemctl >/dev/null 2>&1 || die 'systemd is required'

if ((EUID != 0)); then
    sudo -v
fi

printf 'Installing Ubuntu packages...\n'
cat <<'EOF'
If apt fails because unrelated third-party repositories are missing GPG keys,
you can try these repair commands, then rerun ./vpn_todesk_init.sh:

  sudo install -d -m 0755 /usr/share/keyrings
  wget -q -O - https://dl.google.com/linux/linux_signing_key.pub | sudo tee /etc/apt/trusted.gpg.d/google.asc >/dev/null
  wget -q -O - https://dl.google.com/linux/linux_signing_key.pub | gpg --dearmor | sudo tee /usr/share/keyrings/google-chrome.gpg >/dev/null
  wget -q -O /tmp/DEB-GPG-KEY-scootersoftware.asc https://www.scootersoftware.com/DEB-GPG-KEY-scootersoftware.asc
  sudo cp /tmp/DEB-GPG-KEY-scootersoftware.asc /etc/apt/trusted.gpg.d/
  sudo apt update

EOF
if ! run_root env DEBIAN_FRONTEND=noninteractive apt-get update; then
    cat >&2 <<'EOF'
Warning: apt-get update failed. This is usually caused by unrelated broken
third-party repositories, such as Chrome or Beyond Compare missing GPG keys.
Continuing with apt-get install using the package indexes already available.
EOF
fi
if ! run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
    stunnel4 openssl ca-certificates curl iproute2 python3; then
    cat >&2 <<'EOF'
Error: Ubuntu package installation failed.
Fix the broken apt repositories, then rerun ./vpn_todesk_init.sh.
EOF
    exit 1
fi

[[ -x /usr/bin/stunnel4 ]] || die '/usr/bin/stunnel4 was not installed'
[[ -x /usr/bin/python3 ]] || die '/usr/bin/python3 was not installed'
[[ -f $server_init_script ]] || die "missing Windows init script: $server_init_script"
[[ -f $server_control_script ]] || die "missing Windows control script: $server_control_script"
[[ -f $server_uninit_script ]] || die "missing Windows uninstall script: $server_uninit_script"
[[ -f $proxy_adapter_script ]] || die "missing local proxy adapter: $proxy_adapter_script"
[[ -s $hysteria_linux_binary ]] || die "missing Hysteria Linux binary: $hysteria_linux_binary"
[[ -s $hysteria_windows_binary ]] || die "missing Hysteria Windows binary: $hysteria_windows_binary"
[[ -s $nssm_binary ]] || die "missing NSSM service wrapper: $nssm_binary"

required_pki_files=(
    "$pki_dir/windows-server/server-fullchain.pem"
    "$pki_dir/windows-server/server.key"
    "$pki_dir/windows-server/client-ca.crt"
    "$pki_dir/ubuntu-client/client-fullchain.pem"
    "$pki_dir/ubuntu-client/client.key"
    "$pki_dir/ubuntu-client/server-ca.crt"
)

if [[ ! -e $pki_dir ]]; then
    printf 'Generating a new private mTLS PKI...\n'
    generation_root=$(mktemp -d "$script_dir/.vpn-todesk-pki.XXXXXXXX")
    temporary_dirs+=("$generation_root")
    generate_private_pki "$generation_root/pki-out"
    mv -T -- "$generation_root/pki-out" "$pki_dir"
    rmdir -- "$generation_root"
else
    printf 'Reusing the existing PKI. No key will be regenerated.\n'
fi

for required_file in "${required_pki_files[@]}"; do
    [[ -f $required_file ]] || die "PKI is incomplete: $required_file"
done

hysteria_auth_file="$pki_dir/hysteria-auth.txt"
if [[ ! -f $hysteria_auth_file ]]; then
    printf 'Generating a strong Hysteria UDP password...\n'
    openssl rand -base64 48 >"$hysteria_auth_file"
    chmod 600 "$hysteria_auth_file"
else
    printf 'Reusing the existing Hysteria UDP password.\n'
fi
hysteria_auth=$(tr -d '\r\n' <"$hysteria_auth_file")
[[ ${#hysteria_auth} -ge 32 ]] || die "Hysteria UDP password is too short: $hysteria_auth_file"

openssl verify -purpose sslserver \
    -CAfile "$pki_dir/ubuntu-client/server-ca.crt" \
    "$pki_dir/windows-server/server-fullchain.pem" >/dev/null
openssl verify -purpose sslclient \
    -CAfile "$pki_dir/windows-server/client-ca.crt" \
    "$pki_dir/ubuntu-client/client-fullchain.pem" >/dev/null
openssl x509 -in "$pki_dir/windows-server/server-fullchain.pem" \
    -noout -checkip "$server_ip" >/dev/null
openssl x509 -in "$pki_dir/windows-server/server-fullchain.pem" \
    -noout -checkend 2592000 >/dev/null \
    || die 'the server certificate expires within 30 days'
openssl x509 -in "$pki_dir/ubuntu-client/client-fullchain.pem" \
    -noout -checkend 2592000 >/dev/null \
    || die 'the client certificate expires within 30 days'
openssl x509 -in "$pki_dir/ubuntu-client/client-fullchain.pem" \
    -noout -checkhost "$client_name" >/dev/null

server_cert_key_id=$(openssl x509 -in "$pki_dir/windows-server/server-fullchain.pem" \
    -pubkey -noout \
    | openssl pkey -pubin -outform DER 2>/dev/null | sha256sum | awk '{print $1}')
server_private_key_id=$(openssl pkey -in "$pki_dir/windows-server/server.key" \
    -pubout -outform DER 2>/dev/null | sha256sum | awk '{print $1}')
[[ $server_cert_key_id == "$server_private_key_id" ]] \
    || die 'server certificate and key do not match'

client_cert_key_id=$(openssl x509 -in "$pki_dir/ubuntu-client/client-fullchain.pem" \
    -pubkey -noout \
    | openssl pkey -pubin -outform DER 2>/dev/null | sha256sum | awk '{print $1}')
client_private_key_id=$(openssl pkey -in "$pki_dir/ubuntu-client/client.key" \
    -pubout -outform DER 2>/dev/null | sha256sum | awk '{print $1}')
[[ $client_cert_key_id == "$client_private_key_id" ]] \
    || die 'client certificate and key do not match'

if ! id -u "$service_user" >/dev/null 2>&1; then
    run_root useradd --system --user-group --home-dir /nonexistent \
        --shell /usr/sbin/nologin "$service_user"
fi
[[ $(id -u "$service_user") -ne 0 ]] || die 'refusing to run the client as UID 0'
[[ $(id -gn "$service_user") == "$service_user" ]] \
    || die "existing user $service_user does not have the expected primary group"

temp_dir=$(mktemp -d "$script_dir/.vpn-todesk-runtime.XXXXXXXX")
temporary_dirs+=("$temp_dir")

cat >"$temp_dir/stunnel-client.conf" <<EOF
foreground = yes
pid =
debug = notice
syslog = no
socket = l:TCP_NODELAY=1
socket = r:TCP_NODELAY=1
socket = l:SO_KEEPALIVE=1
socket = r:SO_KEEPALIVE=1

[todesk-socks]
client = yes
accept = 127.0.0.1:$tunnel_local_port
connect = $server_ip:$server_port

cert = $client_install_dir/client-fullchain.pem
key = $client_install_dir/client.key
CAfile = $client_install_dir/server-ca.crt
verifyChain = yes
checkIP = $server_ip

sslVersionMin = TLSv1.2
securityLevel = 2
EOF

cat >"$temp_dir/hysteria-client.yaml" <<EOF
server: $server_ip:$server_port
auth: "$hysteria_auth"

tls:
  ca: $client_install_dir/server-ca.crt
  clientCertificate: $client_install_dir/client-fullchain.pem
  clientKey: $client_install_dir/client.key

quic:
  maxIdleTimeout: 30s
  keepAlivePeriod: 10s

congestion:
  type: bbr
  bbrProfile: conservative

socks5:
  listen: 127.0.0.1:$local_port
  disableUDP: false

http:
  listen: 127.0.0.1:$local_port
EOF

cat >"$temp_dir/$service_name" <<EOF
[Unit]
Description=ToDesk mTLS SOCKS tunnel
Wants=network-online.target
After=network-online.target
StartLimitIntervalSec=30
StartLimitBurst=5

[Service]
Type=simple
User=$service_user
Group=$service_user
ExecStart=/usr/bin/stunnel4 $client_install_dir/stunnel-client.conf
Restart=always
RestartSec=3
TimeoutStopSec=10
KillMode=control-group
NoNewPrivileges=yes
PrivateTmp=yes
PrivateDevices=yes
ProtectSystem=strict
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6
CapabilityBoundingSet=
AmbientCapabilities=

[Install]
WantedBy=multi-user.target
EOF

cat >"$temp_dir/$udp_service_name" <<EOF
[Unit]
Description=ToDesk Hysteria UDP proxy client
Wants=network-online.target
After=network-online.target
StartLimitIntervalSec=30
StartLimitBurst=5

[Service]
Type=simple
User=$service_user
Group=$service_user
ExecStart=$client_install_dir/$hysteria_linux_binary_name --disable-update-check client -c $client_install_dir/hysteria-client.yaml
Restart=always
RestartSec=3
TimeoutStopSec=10
KillMode=control-group
NoNewPrivileges=yes
PrivateTmp=yes
PrivateDevices=yes
ProtectSystem=strict
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6
CapabilityBoundingSet=
AmbientCapabilities=

[Install]
WantedBy=multi-user.target
EOF

cat >"$temp_dir/$proxy_service_name" <<EOF
[Unit]
Description=ToDesk local HTTP proxy adapter
Requires=$service_name
After=$service_name
StartLimitIntervalSec=30
StartLimitBurst=5

[Service]
Type=simple
User=$service_user
Group=$service_user
ExecStart=/usr/bin/python3 $client_install_dir/todesk_proxy_adapter.py --listen-host 127.0.0.1 --listen-port $local_port --socks-host 127.0.0.1 --socks-port $tunnel_local_port
Restart=always
RestartSec=3
TimeoutStopSec=10
KillMode=control-group
NoNewPrivileges=yes
PrivateTmp=yes
PrivateDevices=yes
ProtectSystem=strict
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6
CapabilityBoundingSet=
AmbientCapabilities=

[Install]
WantedBy=multi-user.target
EOF

cat >"$temp_dir/$todesk_autostart_script_name" <<EOF
#!/usr/bin/env bash
set -Eeuo pipefail

tunnel_service_name='$service_name'
proxy_service_name='$proxy_service_name'
udp_service_name='$udp_service_name'
local_port='$local_port'
tunnel_local_port='$tunnel_local_port'

listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:\${local_port}([[:space:]]|$)"
}

tunnel_listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:\${tunnel_local_port}([[:space:]]|$)"
}

show_failure() {
    systemctl status "\$tunnel_service_name" "\$proxy_service_name" --no-pager || true
    journalctl -u "\$tunnel_service_name" -u "\$proxy_service_name" -n 120 --no-pager || true
}

echo 'Stopping any previous managed client instance on the local ToDesk port...'
systemctl stop "\$proxy_service_name" "\$tunnel_service_name" "\$udp_service_name" >/dev/null 2>&1 || true
systemctl reset-failed "\$proxy_service_name" "\$tunnel_service_name" "\$udp_service_name" >/dev/null 2>&1 || true

echo 'Starting the encrypted tunnel...'
systemctl start "\$tunnel_service_name"

for attempt in {1..80}; do
    if systemctl is-active --quiet "\$tunnel_service_name" && tunnel_listener_is_ready; then
        break
    fi
    sleep 0.5
done
if ! tunnel_listener_is_ready; then
    echo "Error: the encrypted tunnel did not listen on 127.0.0.1:\${tunnel_local_port}."
    show_failure
    exit 1
fi

echo 'Starting the local ToDesk proxy adapter...'
systemctl start "\$proxy_service_name"

for attempt in {1..80}; do
    if systemctl is-active --quiet "\$proxy_service_name" && listener_is_ready; then
        echo "ToDesk TCP proxy is ready on 127.0.0.1:\${local_port}."
        exit 0
    fi
    sleep 0.5
done

echo "Error: the local ToDesk proxy adapter did not listen on 127.0.0.1:\${local_port}."
show_failure
exit 1
EOF

cat >"$temp_dir/$todesk_autostart_service_name" <<EOF
[Unit]
Description=Start ToDesk TCP proxy after boot
Wants=network-online.target
After=network-online.target
Before=todeskd.service todesk.service
StartLimitIntervalSec=30
StartLimitBurst=3

[Service]
Type=oneshot
ExecStart=$client_install_dir/$todesk_autostart_script_name
RemainAfterExit=yes
TimeoutStartSec=90
TimeoutStopSec=20

[Install]
WantedBy=multi-user.target
EOF

cat >"$temp_dir/$udp_autostart_script_name" <<EOF
#!/usr/bin/env bash
set -Eeuo pipefail

tcp_tunnel_service_name='$service_name'
tcp_proxy_service_name='$proxy_service_name'
udp_service_name='$udp_service_name'
local_port='$local_port'

listener_is_ready() {
    ss -H -ltn 2>/dev/null | grep -Eq "127\\.0\\.0\\.1:\${local_port}([[:space:]]|$)"
}

show_failure() {
    systemctl status "\$udp_service_name" --no-pager || true
    journalctl -u "\$udp_service_name" -n 160 --no-pager || true
}

echo 'Stopping any previous managed client instance on the local ToDesk port...'
systemctl stop "\$tcp_proxy_service_name" "\$tcp_tunnel_service_name" "\$udp_service_name" >/dev/null 2>&1 || true
systemctl reset-failed "\$tcp_proxy_service_name" "\$tcp_tunnel_service_name" "\$udp_service_name" >/dev/null 2>&1 || true

echo 'Starting the Hysteria UDP tunnel...'
systemctl start "\$udp_service_name"

for attempt in {1..80}; do
    if systemctl is-active --quiet "\$udp_service_name" && listener_is_ready; then
        echo "ToDesk UDP proxy is ready on 127.0.0.1:\${local_port}."
        exit 0
    fi
    sleep 0.5
done

echo "Error: the Hysteria UDP client did not listen on 127.0.0.1:\${local_port}."
show_failure
exit 1
EOF

cat >"$temp_dir/$udp_autostart_service_name" <<EOF
[Unit]
Description=Start ToDesk UDP proxy after boot
Wants=network-online.target
After=network-online.target
Before=todeskd.service todesk.service
StartLimitIntervalSec=30
StartLimitBurst=3

[Service]
Type=oneshot
ExecStart=$client_install_dir/$udp_autostart_script_name
RemainAfterExit=yes
TimeoutStartSec=90
TimeoutStopSec=20

[Install]
WantedBy=multi-user.target
EOF

if ((client_only == 0)); then
package_stage=$(mktemp -d "$script_dir/.vpn-todesk-windows.XXXXXXXX")
temporary_dirs+=("$package_stage")
mkdir -p "$package_stage/certs" "$package_stage/private" "$package_stage/logs"
cat >"$package_stage/stunnel-server.conf" <<EOF
debug = notice
output = C:/ProgramData/ToDeskTlsProxy/logs/stunnel-server.log
taskbar = no
socket = l:TCP_NODELAY=1
socket = r:TCP_NODELAY=1
socket = l:SO_KEEPALIVE=1
socket = r:SO_KEEPALIVE=1

[todesk-socks]
client = no
accept = 0.0.0.0:$server_port
protocol = socks

cert = C:/ProgramData/ToDeskTlsProxy/certs/server-fullchain.pem
key = C:/ProgramData/ToDeskTlsProxy/private/server.key
CAfile = C:/ProgramData/ToDeskTlsProxy/certs/client-ca.crt
verifyChain = yes
requireCert = yes
checkHost = $client_name

sslVersionMin = TLSv1.2
sslVersionMax = TLSv1.3
securityLevel = 2
EOF

cat >"$package_stage/hysteria-server.yaml" <<EOF
listen: 0.0.0.0:$server_port

tls:
  cert: C:/ProgramData/ToDeskTlsProxy/certs/server-fullchain.pem
  key: C:/ProgramData/ToDeskTlsProxy/private/server.key
  clientCA: C:/ProgramData/ToDeskTlsProxy/certs/client-ca.crt
  sniGuard: disable

auth:
  type: password
  password: "$hysteria_auth"

quic:
  maxIdleTimeout: 30s

congestion:
  type: bbr
  bbrProfile: conservative

disableUDP: false
udpIdleTimeout: 180s
EOF

install -m 0644 "$pki_dir/windows-server/server-fullchain.pem" \
    "$package_stage/certs/server-fullchain.pem"
install -m 0644 "$pki_dir/windows-server/client-ca.crt" \
    "$package_stage/certs/client-ca.crt"
install -m 0600 "$pki_dir/windows-server/server.key" \
    "$package_stage/private/server.key"
install -m 0644 "$server_init_script" "$package_stage/vpn_todesk_server_init.bat"
install -m 0644 "$server_control_script" "$package_stage/vpn_todesk_server.bat"
install -m 0644 "$server_uninit_script" "$package_stage/vpn_todesk_server_uninit.bat"
install -m 0755 "$hysteria_windows_binary" "$package_stage/$hysteria_windows_binary_name"
install -m 0755 "$nssm_binary" "$package_stage/$nssm_binary_name"

cat >"$package_stage/WINDOWS_INSTALL.txt" <<'EOF'
Windows Server 2012 one-time setup (run Command Prompt as Administrator):

1. Copy this package's CONTENTS to C:\ProgramData\ToDeskTlsProxy
2. Open Command Prompt as Administrator.
3. Run:

   cd /d C:\ProgramData\ToDeskTlsProxy
   vpn_todesk_server_init.bat

The init BAT detects stunnel for TCP mode. If it is missing, the BAT prints the
official Win64 installer and SHA-256 URLs, then waits for Enter. Hysteria 2 and
NSSM are already included in this package for UDP mode; do not rename either
executable.

Server commands:

   vpn_todesk_server.bat start tcp
   vpn_todesk_server.bat start udp
   vpn_todesk_server.bat start all
   vpn_todesk_server.bat stop tcp
   vpn_todesk_server.bat stop udp
   vpn_todesk_server.bat stop all
   vpn_todesk_server.bat status all
   vpn_todesk_server.bat logs tcp
   vpn_todesk_server.bat logs udp

Without a mode argument, start/stop/status/logs default to tcp.
start enables automatic startup after a Windows reboot for that mode.
stop disables automatic startup after a Windows reboot for that mode.

Uninstall this project from the server:

   vpn_todesk_server_uninit.bat

This uninstall script stops/deletes only vpn-todesk-server and
vpn-todesk-server-udp, removes only this project's firewall rules and package
directory, and leaves stunnel installed.
EOF

# Normalize all Windows command/instruction files to CRLF.
for windows_text in \
    "$package_stage/WINDOWS_INSTALL.txt" \
    "$package_stage/hysteria-server.yaml" \
    "$package_stage/vpn_todesk_server_init.bat" \
    "$package_stage/vpn_todesk_server.bat" \
    "$package_stage/vpn_todesk_server_uninit.bat"; do
    sed -i 's/\r$//' "$windows_text"
    sed -i 's/$/\r/' "$windows_text"
done

printf 'This directory was generated by vpn_todesk_init.sh.\r\n' \
    >"$package_stage/PACKAGE_GENERATED.txt"
printf 'stunnel writes its Windows service log in this directory.\n' \
    >"$package_stage/logs/README.txt"
(
    cd -- "$package_stage"
    sha256sum stunnel-server.conf hysteria-server.yaml \
        "$hysteria_windows_binary_name" "$nssm_binary_name" \
        certs/server-fullchain.pem certs/client-ca.crt \
        private/server.key vpn_todesk_server_init.bat vpn_todesk_server.bat \
        vpn_todesk_server_uninit.bat WINDOWS_INSTALL.txt PACKAGE_GENERATED.txt \
        >SHA256SUMS.txt
)
chmod 0700 "$package_stage" "$package_stage/certs" \
    "$package_stage/private" "$package_stage/logs"

if [[ -e $windows_package ]]; then
    [[ -d $windows_package && ! -L $windows_package ]] \
        || die "refusing to replace unsafe package path: $windows_package"
    [[ -f $windows_package/PACKAGE_GENERATED.txt ]] \
        || die "refusing to replace a package not created by this script: $windows_package"
    rm -rf -- "$windows_package"
fi
mv -T -- "$package_stage" "$windows_package"

printf '\nNow copy all files inside %s to Windows server %s at C:\\ProgramData\\ToDeskTlsProxy, then run vpn_todesk_server_init.bat as Administrator. After the server init succeeds, return here and press Enter to continue: ' \
    "$windows_package" "$server_ip"
if ! IFS= read -r _ </dev/tty; then
    die 'an interactive terminal is required to confirm the Windows copy step'
fi
printf '\nContinuing with the Ubuntu client installation...\n'
else
printf 'Client-only mode: skipping Windows package generation and copy prompt.\n'
fi

run_root install -d -m 0750 -o root -g "$service_user" "$client_install_dir"
run_root install -m 0755 -o root -g root \
    "$proxy_adapter_script" "$client_install_dir/todesk_proxy_adapter.py"
run_root install -m 0755 -o root -g root \
    "$hysteria_linux_binary" "$client_install_dir/$hysteria_linux_binary_name"
run_root install -m 0755 -o root -g root \
    "$temp_dir/$todesk_autostart_script_name" "$client_install_dir/$todesk_autostart_script_name"
run_root install -m 0755 -o root -g root \
    "$temp_dir/$udp_autostart_script_name" "$client_install_dir/$udp_autostart_script_name"
run_root install -m 0640 -o root -g "$service_user" \
    "$pki_dir/ubuntu-client/client-fullchain.pem" "$client_install_dir/client-fullchain.pem"
run_root install -m 0640 -o root -g "$service_user" \
    "$pki_dir/ubuntu-client/client.key" "$client_install_dir/client.key"
run_root install -m 0640 -o root -g "$service_user" \
    "$pki_dir/ubuntu-client/server-ca.crt" "$client_install_dir/server-ca.crt"
run_root install -m 0640 -o root -g "$service_user" \
    "$temp_dir/stunnel-client.conf" "$client_install_dir/stunnel-client.conf"
run_root install -m 0640 -o root -g "$service_user" \
    "$temp_dir/hysteria-client.yaml" "$client_install_dir/hysteria-client.yaml"
run_root install -m 0644 -o root -g root "$temp_dir/$service_name" "$unit_path"
run_root install -m 0644 -o root -g root "$temp_dir/$proxy_service_name" "$proxy_unit_path"
run_root install -m 0644 -o root -g root \
    "$temp_dir/$todesk_autostart_service_name" "$todesk_autostart_unit_path"
run_root install -m 0644 -o root -g root "$temp_dir/$udp_service_name" "$udp_unit_path"
run_root install -m 0644 -o root -g root \
    "$temp_dir/$udp_autostart_service_name" "$udp_autostart_unit_path"
run_root systemctl daemon-reload

printf '\nInitialization completed.\n'
if ((client_only == 0)); then
    printf 'Windows package: %s\n' "$windows_package"
fi
printf 'Then run: %s/vpn_todesk.sh start udp\n' "$script_dir"
printf 'TCP fallback is still available with: %s/vpn_todesk.sh start tcp\n' "$script_dir"
printf 'Keep this UDP password file private: %s\n' "$hysteria_auth_file"
printf 'Do not upload this CA directory: %s/offline-ca\n' "$pki_dir"
