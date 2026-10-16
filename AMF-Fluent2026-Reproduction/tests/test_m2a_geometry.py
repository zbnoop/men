import copy
import importlib.util
import json
import pathlib
import unittest
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("geom", ROOT/"geometry/m2a_spaceclaim.py")
geom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(geom)

class GeometryGateTests(unittest.TestCase):
    def setUp(self):
        self.c = json.loads((ROOT/"config/m2a_geometry_demo.json").read_text())
    def test_demo_volume_contact(self):
        r = geom.validate_config(self.c)
        self.assertAlmostEqual(r["solid_volume_m3"], 0.000036)
        self.assertAlmostEqual(r["fluid_volume_m3"], 0.000084)
        self.assertAlmostEqual(r["contact_area_m2"], 0.0024)
    def test_missing_production(self):
        c = json.loads((ROOT/"config/m2a_geometry_production.json").read_text())
        with self.assertRaisesRegex(ValueError, "Missing production"):
            geom.validate_config(c)
    def test_production_adapter_not_silently_demo(self):
        c = json.loads((ROOT/"config/m2a_geometry_production.json").read_text())
        c["critical_dimensions"] = dict.fromkeys(c["critical_dimensions"], 1)
        with self.assertRaisesRegex(ValueError, "NOT released"):
            geom.validate_config(c)
    def test_bad_inputs(self):
        for key, value in [("units", "mm"), ("mode", "demo"), ("demo_only", False),
                           ("tolerance_m", 0), ("tolerance_m", float("nan"))]:
            with self.subTest(key=key):
                c = copy.deepcopy(self.c); c[key] = value
                with self.assertRaises(ValueError):
                    geom.validate_config(c)
    def test_overlap_gap_partial_and_nonfinite(self):
        for key, index, value in [("fluid_bbox",1,0.014), ("fluid_bbox",1,0.016),
                                  ("fluid_bbox",3,0.10), ("solid_bbox",3,0),
                                  ("solid_bbox",0,float("nan")),
                                  ("solid_bbox",0,float("inf"))]:
            with self.subTest(value=value):
                c = copy.deepcopy(self.c); c[key][index] = value
                with self.assertRaises(ValueError):
                    geom.validate_config(c)
    def test_provenance(self):
        a = json.loads((ROOT/"config/m2a_geometry_audit.json").read_text(encoding="utf-8"))
        ids = [p["id"] for p in a["parameters"]]
        self.assertEqual(len(ids),len(set(ids)))
        for p in a["parameters"]:
            self.assertIn(p["status"], ["paper","derived","unknown","assumption"])
            self.assertTrue(p["location"] and p["unit"] and p["source"])
            if p["status"] == "unknown":
                self.assertIsNone(p["value"])
    def test_production_material_gate_preserved(self):
        p = json.loads((ROOT/"config/paper_parameters.json").read_text())
        for k in ("density","cp_solid","cp_liquid","latent_heat"):
            self.assertIsNone(p["thermal"][k]["value"])
        c = json.loads((ROOT/"config/m2a_material_candidates.json").read_text(encoding="utf-8"))
        self.assertFalse(c["production_approved"])
    def test_package_matches_source(self):
        with zipfile.ZipFile(ROOT/"geometry/m2a_spaceclaim.scscript") as z:
            self.assertEqual(z.read("_script"), (ROOT/"geometry/m2a_spaceclaim.py").read_bytes())
            self.assertEqual(z.read("_apiVersion"), b"261")

if __name__ == "__main__":
    unittest.main(verbosity=2)
