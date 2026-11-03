"""Real NetSender automatic discovery and business callbacks after gateway replacement."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import time
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from client_background_process_test import Windows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    location=parser.add_mutually_exclusive_group(required=True)
    location.add_argument('--sdk',type=Path)
    location.add_argument('--bin',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    windows=Windows()
    windows.require_free_client()
    windows.require_free_client('NetServerManager')
    windows.require_free_client('NetClientManagerSupervisor')
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    binaries=args.bin.resolve() if args.bin else args.sdk.resolve()/'bin'
    children=[]
    def start(name,exe,arguments,pattern):
        log=(output/(name+'.log')).open('wb')
        process=subprocess.Popen([str(binaries/exe)]+arguments,cwd=output,stdout=log,stderr=subprocess.STDOUT,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        children.append((process,log))
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            text=(output/(name+'.log')).read_text(errors='replace')
            match=re.search(pattern,text)
            if match: return process,match
            if process.poll() is not None: raise RuntimeError(name+' stopped before ready: '+text)
            time.sleep(.02)
        raise RuntimeError(name+' startup deadline')
    def gateway(name,port):
        return start(name,'NetServerManager1.1.exe',['--listen=127.0.0.1','--advertise=127.0.0.1','--port='+str(port),
            '--identity-directory='+str(output/'identity'),'--run-for-ms=40000','--stop-file='+str(output/(name+'.stop'))],
            r'GATEWAY_READY port=(\d+) pid=(\d+)')
    def service(name,pid):
        return start(name,'NetServerManager1.1.exe',['--self-test-service='+str(pid),
            '--stop-file='+str(output/(name+'.stop')),'--run-for-ms=40000'],r'SERVICE_PROBE_READY')
    def client(name,port):
        return start(name,'NetClientManager1.1.exe',['--address=127.0.0.1','--port='+str(port),
            '--cache-file='+str(output/'client-nodes.cache'),'--run-for-ms=40000',
            '--stop-file='+str(output/(name+'.stop'))],r'CLIENT_GATEWAY_READY')
    try:
        server,ready=gateway('server-a',0)
        port=int(ready[1])
        worker,_=service('service-a',server.pid)
        desktop,_=client('desktop-a',port)
        endpoint,_=start('desktop-endpoint','NetWorkTest1.0.exe',['--recovery-desktop=0'],r'RECOVERY_ROUND count=1 ')
        (output/'desktop-a.stop').write_text('stop')
        assert desktop.wait(timeout=5)==0
        client('desktop-b',port)
        assert endpoint.wait(timeout=15)==0,(output/'desktop-endpoint.log').read_text()
        text=(output/'desktop-endpoint.log').read_text()
        assert 'RECOVERY_CLIENT_PASS desktop=1' in text
        native,_=start('native-endpoint','NetWorkTest1.0.exe',['--recovery-native=127.0.0.1:'+str(port)],r'RECOVERY_ROUND count=1 ')
        (output/'server-a.stop').write_text('stop')
        assert server.wait(timeout=5)==0
        worker.wait(timeout=5)
        replacement,_=gateway('server-b',port)
        service('service-b',replacement.pid)
        assert native.wait(timeout=15)==0,(output/'native-endpoint.log').read_text()
        assert 'RECOVERY_CLIENT_PASS desktop=0' in (output/'native-endpoint.log').read_text()
        for name in ['desktop-b','service-b','server-b']: (output/(name+'.stop')).write_text('stop')
        for process,log in children:
            process.wait(timeout=5)
        (output/'result.json').write_text(json.dumps({'pass':True,'desktopAutomaticPid':True,
            'clientReplacementCallbackAndNewGeneration':True,'serverReplacementCallbackAndNewGeneration':True,
            'fullRequestResponseAfterRecovery':True},indent=2))
        print('PASS gateway replacement: NetSender rediscovery, close/ready callbacks, fresh response')
    finally:
        for process,log in children:
            if process.poll() is None: process.kill()
            process.wait(timeout=5)
            log.close()


if __name__=='__main__': main()