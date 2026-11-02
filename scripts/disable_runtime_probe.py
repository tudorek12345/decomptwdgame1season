"""Move the optional auto-start fixture out of Pack; preserve it for reproduction."""
from pathlib import Path
import datetime,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
def main():
    source=ROOT/'runtime/Pack/default/0_000000_recovery_probe.ttarch'
    if not source.exists():print('Runtime probe already disabled');return
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    target=ROOT/'build/disabled-runtime-fixtures'/(stamp+'_'+source.name)
    target.parent.mkdir(parents=True,exist_ok=True)
    assert source.resolve().is_relative_to(ROOT/'runtime/Pack/default')
    assert target.resolve().is_relative_to(ROOT/'build/disabled-runtime-fixtures')
    digest=hashlib.sha256(source.read_bytes()).hexdigest();source.rename(target)
    report=ROOT/'reports/runtime-probe-build.json'
    record=json.loads(report.read_text());record.update(enabled=False,disabled_archive=str(target),disabled_sha256=digest)
    report.write_text(json.dumps(record,indent=2)+'\n');print('Probe disabled; preserved at '+str(target))
if __name__=='__main__':main()
