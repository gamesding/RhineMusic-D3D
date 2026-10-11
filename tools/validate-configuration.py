"""Verify the portable configuration app and first-device player settings."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/configuration-delivery'
sys.stdout.reconfigure(encoding='utf-8')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    delivery = OUT / '独立配置程序'
    delivery.mkdir(exist_ok=True)
    configuration = delivery / 'RhineConfiguration.exe'
    shutil.copy2(ROOT / 'Release/RhineConfiguration.exe', configuration)
    assert sorted(p.name for p in delivery.iterdir()) == ['RhineConfiguration.exe']
    env = os.environ.copy()
    for name in list(env):
        if name.upper().startswith(('RHINE_', 'SONY_WALKMAN_', 'PYTHON', 'NODE_')) or name.upper() == 'MUSIC_DATA_DIR':
            env.pop(name, None)
    system = Path(env.get('SystemRoot', 'C:/Windows'))
    env['PATH'] = str(system / 'System32') + os.pathsep + str(system)
    env['RHINE_RUNTIME_ROOT'] = str(OUT / ('runtime-' + uuid.uuid4().hex[:12]))
    env['RHINE_D3D_DEBUG'] = '1'
    config_report = OUT / 'configuration.json'
    subprocess.run([str(configuration), '--self-test', '--report', str(config_report)],
                   cwd=delivery, env=env, creationflags=0x08000000, timeout=90, check=True)
    configuration_data = json.loads(config_report.read_text('utf-8'))
    assert configuration_data['ok'] and configuration_data['checks'] >= 152
    assert sorted(p.name for p in delivery.iterdir()) == ['RhineConfiguration.exe']
    print('Portable configuration UI, policies and atomic save: PASS (' + str(configuration_data['checks']) + ' checks)', flush=True)
    hardware = configuration_data['hardware']['adapters']
    assert hardware, 'No compatible renderer was detected'
    gpu = next((a for a in hardware if a['dx11'] and not a['software']), None)
    profiles = []

    def profile(name, adapter, api, upscaler, reflections, scale=100):
        preferences = {'configurationMode': 'custom', 'graphicsApi': api,
                       'graphicsAdapter': adapter['luid'], 'graphicsSoftware': adapter['software'],
                       'upscaler': upscaler, 'upscaleQuality': 'balanced',
                       'reflections': reflections, 'reflectionQuality': 'performance',
                       'quality': 'performance', 'renderQuality': {
                           'scale': scale, 'pixelRatio': 1, 'antialias': 'off', 'shadows': 0,
                           'aoSamples': 0, 'aoResolution': .5, 'depthOfField': 0,
                           'transmission': .25, 'anisotropy': 4}}
        profiles.append((name, preferences))

    if gpu:
        profile('dx11-custom-before-first-device', gpu, 'dx11', 'native', 'off', 65)
        profile('dx11-fsr', gpu, 'dx11', 'fsr', 'off')
    dx12 = next((a for a in hardware if a['dx12'] and not a['software']), None)
    if dx12:
        profile('dx12-fsr-ssr', dx12, 'dx12', 'fsr', 'ssr')
    rtx = next((a for a in reversed(hardware) if a['dx12'] and a['spectralEligible'] and a['dlssEligible']), None)
    if rtx:
        profile('dx12-dlss-rtx-selected-adapter', rtx, 'dx12', 'dlss', 'rtx')
    software = next((a for a in hardware if a['software'] and a['dx11']), None)
    if software:
        profile('software-dx11-compatibility', software, 'dx11', 'native', 'off', 50)
    checks = []
    package = json.loads((ROOT / 'build/package/package-manifest.json').read_text('utf-8'))
    assert hashlib.file_digest(configuration.open('rb'), 'sha256').hexdigest() == package['configurationSha256']
    for name, preferences in profiles:
        print('Testing saved startup profile: ' + name, flush=True)
        fixture = OUT / (name + '-preferences.json')
        report = OUT / (name + '.json')
        fixture.write_text(json.dumps(preferences), 'utf-8')
        size = (640, 480) if preferences['graphicsSoftware'] else (960, 640)
        args = [str(ROOT / 'Release/RhineMusic.exe'), '--self-test', '--demo', '--reduced',
                '--startup-profile', str(fixture), '--view', 'browse', '--duration', '2',
                '--width', str(size[0]), '--height', str(size[1]),
                '--report', str(report), '--image', str(OUT / (name + '.png'))]
        subprocess.run(args, cwd=ROOT, env=env, creationflags=0x08000000, timeout=150, check=True)
        result = json.loads(report.read_text('utf-8'))
        assert result['ok'], result.get('error', name)
        initial, final = result['startupGraphics'], result['graphics']
        expected_api = 'Direct3D 12' if preferences['graphicsApi'] == 'dx12' else 'Direct3D 11'
        for status in [initial, final]:
            assert status['api'] == expected_api, (name, status['api'])
            assert status['software'] == preferences['graphicsSoftware'], name
            assert abs(status['renderScale'] - preferences['renderQuality']['scale'] / 100) < .001, name
            assert status['shadows'] == 0 and status['aoSamples'] == 0, name
            assert abs(status['transmissionResolution'] - .25) < .001, name
            assert status['reflectionRequested'] == preferences['reflections'], name
            if not preferences['graphicsSoftware']:
                assert status['adapter'] == preferences['graphicsAdapter'], name
        assert final['debugErrors'] == 0 and final['debugDiscarded'] == 0, name
        assert result['graphicsRecoveries'] == 0, name
        if preferences['reflections'] == 'rtx':
            assert final['reflectionActive'] == 'DXR' and final['spectralCausticsActive'], name
        if preferences['upscaler'] != 'native':
            assert final['upscaler']['active'], name
            assert preferences['upscaler'].upper() in final['upscaler']['provider'].upper(), name
        else:
            assert not final['upscaler']['active'], name
        checks.append({'name': name, 'ok': True, 'api': initial['api'],
                       'adapter': initial['adapter'], 'software': initial['software'],
                       'renderScale': initial['renderScale'], 'checks': len(result['checks']),
                       'report': str(report.relative_to(ROOT))})
    result = {'ok': True, 'version': '0.1', 'configurationChecks': configuration_data['checks'],
              'portableConfiguration': True, 'firstDeviceProfiles': checks,
              'hardware': configuration_data['hardware'], 'physicalOldGpuTested': False,
              'configurationSha256': package['configurationSha256'], 'playerSha256': package['exeSha256']}
    (OUT / 'validation.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', 'utf-8')
    print('Configuration delivery and first-device preferences: PASS (' + str(len(checks)) + ' render profiles)', flush=True)


if __name__ == '__main__':
    main()
