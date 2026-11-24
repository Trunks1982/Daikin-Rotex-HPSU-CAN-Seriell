"""Config validation: entities calculated from others need those entities in the yaml.

Runs `esphome config` (validation only, nothing is compiled) on hpsu_host.yaml plus extra entities.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

HERE = Path(__file__).parent


def esphome_config(tmp_path: Path, extra_entities: str) -> subprocess.CompletedProcess:
    yaml = (HERE / "hpsu_host.yaml").read_text()
    # the copy lives elsewhere: absolute paths for the local components
    yaml = yaml.replace("path: ../../components", f"path: {(HERE / '../../components').resolve()}")
    yaml = yaml.replace("path: components", f"path: {(HERE / 'components').resolve()}")
    marker = "  entities:\n"
    assert marker in yaml
    yaml = yaml.replace(marker, marker + extra_entities)
    config = tmp_path / "config.yaml"
    config.write_text(yaml)

    esphome = shutil.which("esphome") or str(Path(sys.executable).parent / "esphome")
    env = {**os.environ, "ESPHOME_DATA_DIR": str(tmp_path / "data")}
    return subprocess.run([esphome, "config", str(config)], cwd=tmp_path, env=env,
                          capture_output=True, text=True, timeout=120)


def test_host_yaml_is_valid(tmp_path):
    result = esphome_config(tmp_path, "")
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("entity, missing", [
    ("tv_tvbh_delta", "tv, tvbh"),
    ("tvbh_tr_delta", "tvbh, tr"),
    ("vorlauf_soll_tv_delta", "target_supply_temperature, tv"),
    ("thermal_power", "flow_rate, tv, tr"),
    ("temperature_spread", "tv, tr"),
    ("tv_max", "tv"),
])
def test_missing_dependency_fails(tmp_path, entity, missing):
    result = esphome_config(tmp_path, f"    {entity}:\n      name: X\n")
    assert result.returncode != 0
    assert f"{entity} needs the entities: {missing}" in result.stdout + result.stderr


def test_regulated_setpoint_needs_its_inputs(tmp_path):
    # hpsu_host.yaml has mode_of_operating, the rest is missing
    result = esphome_config(tmp_path, "    supply_setpoint_regulated:\n      name: X\n")
    assert result.returncode != 0
    assert ("supply_setpoint_regulated needs the entities: status_kompressor, max_target_flow_temp, tv, "
            "target_supply_temperature") in result.stdout + result.stderr


def test_dependencies_present_is_valid(tmp_path):
    result = esphome_config(tmp_path, "    tv:\n      name: Tv\n    tvbh:\n      name: Tvbh\n"
                                      "    tv_tvbh_delta:\n      name: X\n")
    assert result.returncode == 0, result.stdout + result.stderr
