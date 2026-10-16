# -*- coding: utf-8 -*-
# SpaceClaim 2026 R1 / V261, IronPython-compatible; v1 preparation only.
# Set AMF_M2_CONFIG to an absolute independent JSON path before launch.
# No production geometry fallback exists.
from __future__ import print_function
import json
import math
import os
import sys

try:
    string_types = (basestring,)  # IronPython 2 JSON returns unicode
except NameError:
    string_types = (str,)

def require(ok, message):
    if not ok:
        raise ValueError(message)

def box_volume(b):
    v = 1.0
    for i in range(3):
        v *= b[i+3] - b[i]
    return v

def validate_config(c):
    require(c.get("schema_version") == 1, "Unsupported schema")
    require(c.get("units") == "m", "SI metres required")
    if c.get("mode") == "production":
        dims = c.get("critical_dimensions", {})
        needed = ("inner_length", "inner_width", "inner_height", "charge_profile",
                  "burner_centers", "nozzle_profiles", "auxiliary_flue")
        missing = [k for k in needed if dims.get(k) is None]
        require(not missing, "Missing production dimensions: " + ", ".join(missing))
        raise ValueError("Production topology adapter is NOT released; audited dimensions and M2-B review required")
    require(c.get("mode") == "DEMO" and c.get("demo_only") is True,
            "Explicit DEMO isolation required")
    require(c.get("geometry_family") == "two_boxes", "Unsupported geometry family")
    tol = c.get("tolerance_m")
    require(isinstance(tol, (int, float)) and 0 < tol <= 1e-7, "Invalid tolerance")
    for key in ("solid_bbox", "fluid_bbox"):
        b = c.get(key)
        require(isinstance(b, list) and len(b) == 6, "Six bbox coordinates required")
        require(all(isinstance(v, (int, float)) and not isinstance(v, bool)
                    and not math.isnan(v) and not math.isinf(v) for v in b),
                "Finite numeric coordinates required")
        require(all(b[i+3]-b[i] > 100*tol for i in range(3)), "Degenerate box")
    s, f = c["solid_bbox"], c["fluid_bbox"]
    require(f[1] >= s[4], "Volume overlap forbidden")
    require(abs(f[1]-s[4]) <= tol, "Interface gap")
    require(all(abs(s[i]-f[i]) <= tol for i in (0, 2, 3, 5)),
            "Full matching interface required; partial contact unsupported")
    return {"solid_volume_m3": box_volume(s), "fluid_volume_m3": box_volume(f),
            "contact_area_m2": (s[3]-s[0])*(s[5]-s[2]),
            "overlap_volume_m3": 0.0,
            "evidence": "analytic input-contract checks only; NOT kernel or mesh validation"}

def build(c, root, save):
    expected = validate_config(c)  # BEFORE any CAD mutation or imports
    out, report = c.get("output_model"), c.get("report_path")
    for p in (out, report):
        require(isinstance(p, string_types) and os.path.isabs(p), "Explicit absolute output paths required")
        require(not os.path.exists(p), "Refusing overwrite: " + p)
        require(os.path.isdir(os.path.dirname(p)), "Create run directory explicitly")
    require(os.path.normcase(out) != os.path.normcase(report), "Distinct outputs required")
    require(out.lower().endswith(".scdocx"), "Native scdocx output required")
    require(len(list(root.GetAllBodies())) == 0, "New empty document required")
    from SpaceClaim.Api.V261 import DesignBody, Matrix, Vector, Group, IDocObject
    from SpaceClaim.Api.V261.Geometry import Plane, PointUV, RectangleProfile
    from SpaceClaim.Api.V261.Modeler import Body as ModelerBody
    from System.Collections.Generic import List
    result = {"target": "SpaceClaim 2026 R1 API V261", "demo_only": True,
              "status": "started", "analytic": expected, "bodies": [], "groups": [],
              "mesh_validation": "NOT_RUN", "reopen_validation": "NOT_RUN"}
    tol = c["tolerance_m"]
    def bbox(shape):
        q = shape.GetBoundingBox(Matrix.Identity)
        return [float(q.MinCorner.X), float(q.MinCorner.Y), float(q.MinCorner.Z),
                float(q.MaxCorner.X), float(q.MaxCorner.Y), float(q.MaxCorner.Z)]
    def close(a, b):
        return abs(a-b) <= tol
    def group(name, members):
        require(len(members) > 0, "Empty boundary: " + name)
        items = List[IDocObject]()
        for m in members:
            items.Add(m)
        g = Group.Create(root, name, items)
        require(g.Name == name and len(list(g.Members)) == len(members),
                "Group name/membership mismatch: " + name)
        result["groups"].append({"name": name, "count": len(members)})
    try:
        contact_areas = []
        for role in ("solid", "fluid"):
            b = c[role+"_bbox"]
            shape = ModelerBody.ExtrudeProfile(RectangleProfile(
                Plane.PlaneXY, b[3]-b[0], b[4]-b[1], PointUV.Create(0, 0), 0), b[5]-b[2])
            require(shape is not None and shape.IsClosed, "Open body")
            body = DesignBody.Create(root, "DEMO_charge_solid" if role == "solid"
                                     else "DEMO_furnace_fluid", shape)
            body.Transform(Matrix.CreateTranslation(Vector.Create(
                (b[0]+b[3])/2, (b[1]+b[4])/2, b[2])))
            require(all(close(a, z) for a, z in zip(bbox(body.Shape), b)), "Kernel bbox mismatch")
            volume = float(body.Shape.Volume)
            require(abs(volume-box_volume(b)) <= box_volume(b)*1e-7, "Kernel volume mismatch")
            faces = list(body.Faces)
            require(len(faces) == 6, "Unexpected box topology")
            face_by_side = {}
            for axis in range(3):
                for side in (0, 1):
                    pos = b[axis+3*side]
                    matches = [face for face in faces if close(bbox(face.Shape)[axis], pos)
                               and close(bbox(face.Shape)[axis+3], pos)]
                    require(len(matches) == 1, "Non-unique semantic plane selector")
                    face_by_side[(axis, side)] = matches[0]
            require(len(set(face_by_side.values())) == 6, "Duplicate face classification")
            interface = face_by_side[(1, 1 if role == "solid" else 0)]
            area = float(interface.Shape.Area)
            require(abs(area-expected["contact_area_m2"]) <= expected["contact_area_m2"]*1e-7,
                    "Kernel interface area mismatch")
            contact_areas.append(area)
            group("DEMO_charge_interface_01_"+role, [interface])
            excluded = [interface]
            if role == "fluid":
                inlet, outlet = face_by_side[(0, 0)], face_by_side[(0, 1)]
                group("DEMO_gas_inlet_01", [inlet])
                group("DEMO_gas_outlet_01", [outlet])
                excluded += [inlet, outlet]
            group("DEMO_wall_"+role, [face for face in faces if face not in excluded])
            result["bodies"].append({"name": body.Name, "volume_m3": volume,
                                     "bbox_m": bbox(body.Shape), "face_count": len(faces)})
        require(len(list(root.GetAllBodies())) == 2, "Unexpected body count")
        require(abs(contact_areas[0]-contact_areas[1]) <= expected["contact_area_m2"]*1e-7,
                "Contact mismatch")
        # For two closed axis-aligned six-face boxes the checked bboxes/volumes
        # and disjoint y interiors prove zero volumetric overlap. Not general CSG.
        result["cad_topology"] = "two closed boxes; paired coincident contact; no shared topology claim"
        saved = save(out)
        require(saved.Success and os.path.isfile(out), "Native save failed")
        result["status"] = "CAD_CONSTRUCTION_CHECKS_PASSED_NOT_MESH_VALIDATED"
    except Exception as exc:
        result["status"] = "FAILED"
        result["error"] = str(exc)
        raise
    finally:
        with open(report, "w") as handle:
            json.dump(result, handle, indent=2, sort_keys=True)

def main():
    path = os.environ.get("AMF_M2_CONFIG")
    require(path and os.path.isabs(path), "Set absolute AMF_M2_CONFIG")
    with open(path, "r") as handle:
        c = json.load(handle)
    if "--validate-only" in sys.argv:
        print(json.dumps(validate_config(c), sort_keys=True))
    else:
        # These are scripting-host functions; plain CPython cannot build CAD.
        build(c, GetRootPart(), DocumentSave.Execute)

if __name__ == "__main__":
    main()
