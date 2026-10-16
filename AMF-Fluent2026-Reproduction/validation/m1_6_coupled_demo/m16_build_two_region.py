# SpaceClaim/SCDM V261 script for the isolated M1.6 two-region DEMO.
#
# Geometry contract (SI units):
#   x: 0..0.10 m, y: 0..0.03 m, z: 0..0.01 m
#   SolidDomain: y=0..0.01 m
#   FluidDomain: y=0.01..0.03 m
# The two bodies touch at y=0.01 m.  They are intentionally kept as
# separate design bodies so Workbench can create separate Fluent cell zones.
# These dimensions and all material values are DEMO-only assumptions.

from __future__ import print_function

import datetime
import json
import os

from SpaceClaim.Api.V261 import *
from SpaceClaim.Api.V261.Geometry import Plane, PointUV, RectangleProfile
from SpaceClaim.Api.V261.Modeler import Body as ModelerBody


CASE_ID = "m16_two_region_coupled_demo"
SPACECLAIM_RELEASE = "ANSYS 2026 R1 / API V261"
LENGTH_X_M = 0.10
SOLID_HEIGHT_M = 0.01
FLUID_HEIGHT_M = 0.02
DEPTH_Z_M = 0.01
INTERFACE_Y_M = SOLID_HEIGHT_M
OUTPUT_MODEL_PATH = "C:/Users/admin/m16_demo/m16_two_region_v2.scdocx"
REPORT_PATH = "C:/Users/admin/m16_demo/m16_geometry_report_v2.json"


def body_bbox_m(body):
    box = body.Shape.GetBoundingBox(Matrix.Identity)
    return [
        float(box.MinCorner.X), float(box.MinCorner.Y), float(box.MinCorner.Z),
        float(box.MaxCorner.X), float(box.MaxCorner.Y), float(box.MaxCorner.Z),
    ]


def near(a, b, tol=1.0e-9):
    return abs(float(a) - float(b)) <= tol


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def make_box(x0, y0, z0, x1, y1, z1, name):
    require(x1 > x0 and y1 > y0 and z1 > z0, "Invalid box dimensions: %s" % name)
    profile = RectangleProfile(
        Plane.PlaneXY,
        x1 - x0,
        y1 - y0,
        PointUV.Create(0.0, 0.0),
        0.0,
    )
    modeler_body = ModelerBody.ExtrudeProfile(profile, z1 - z0)
    require(modeler_body is not None and modeler_body.IsClosed,
            "Failed to create closed modeler body: %s" % name)
    design_body = DesignBody.Create(GetRootPart(), name, modeler_body)
    translation = Matrix.CreateTranslation(
        Vector.Create(0.5 * (x0 + x1), 0.5 * (y0 + y1), z0)
    )
    design_body.Transform(translation)
    return design_body


def write_report(status, bodies, error=None):
    report = {
        "schema_version": 1,
        "case_id": CASE_ID,
        "status": status,
        "generated_at_utc": datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
        "target": {
            "spaceclaim_release": SPACECLAIM_RELEASE,
            "api_version": "V261",
            "units": "m",
        },
        "contract": {
            "bbox_m": [0.0, 0.0, 0.0, LENGTH_X_M,
                       SOLID_HEIGHT_M + FLUID_HEIGHT_M, DEPTH_Z_M],
            "interface_y_m": INTERFACE_Y_M,
            "solid_name": "SolidDomain",
            "fluid_name": "FluidDomain",
            "solid_bbox_m": [0.0, 0.0, 0.0, LENGTH_X_M, SOLID_HEIGHT_M, DEPTH_Z_M],
            "fluid_bbox_m": [0.0, SOLID_HEIGHT_M, 0.0, LENGTH_X_M,
                             SOLID_HEIGHT_M + FLUID_HEIGHT_M, DEPTH_Z_M],
        },
        "bodies": bodies,
        "outputs": {
            "native_model_path": OUTPUT_MODEL_PATH,
            "report_path": REPORT_PATH,
        },
        "evidence_boundary": (
            "This is SpaceClaim construction evidence only. Fluent mesh zoning, "
            "shared topology and Coupled Wall behavior require separate validation."
        ),
    }
    if error is not None:
        report["error"] = error
    directory = os.path.dirname(REPORT_PATH)
    if not os.path.isdir(directory):
        os.makedirs(directory)
    with open(REPORT_PATH, "w") as handle:
        handle.write(json.dumps(report, ensure_ascii=True, indent=2, sort_keys=True))
        handle.write("\n")


def run():
    try:
        existing = list(GetRootPart().GetAllBodies())
        require(len(existing) == 0, "Expected a new empty document, found %d bodies" % len(existing))
        require(not os.path.exists(OUTPUT_MODEL_PATH),
                "Refusing to overwrite existing model: %s" % OUTPUT_MODEL_PATH)

        solid = make_box(0.0, 0.0, 0.0, LENGTH_X_M, SOLID_HEIGHT_M, DEPTH_Z_M,
                         "SolidDomain")
        fluid = make_box(0.0, SOLID_HEIGHT_M, 0.0, LENGTH_X_M,
                         SOLID_HEIGHT_M + FLUID_HEIGHT_M, DEPTH_Z_M,
                         "FluidDomain")

        bodies = list(GetRootPart().GetAllBodies())
        require(len(bodies) == 2, "Expected two design bodies, found %d" % len(bodies))
        records = []
        for body in bodies:
            bbox = body_bbox_m(body)
            records.append({
                "name": body.Name,
                "bbox_m": bbox,
                "volume_m3": float(body.Shape.Volume),
                "face_count": len(list(body.Faces)),
            })

        solid_record = [r for r in records if r["name"] == "SolidDomain"]
        fluid_record = [r for r in records if r["name"] == "FluidDomain"]
        require(len(solid_record) == 1 and len(fluid_record) == 1,
                "Required semantic body names were not preserved")
        require(near(solid_record[0]["bbox_m"][4], INTERFACE_Y_M),
                "Solid upper face is not at the declared interface")
        require(near(fluid_record[0]["bbox_m"][1], INTERFACE_Y_M),
                "Fluid lower face is not at the declared interface")

        save_result = DocumentSave.Execute(OUTPUT_MODEL_PATH)
        require(save_result.Success and os.path.isfile(OUTPUT_MODEL_PATH),
                "Native SpaceClaim save failed")
        write_report("passed", records)
        print("M1.6 geometry PASS: two bodies, shared y=%g m interface" % INTERFACE_Y_M)
    except Exception as exc:
        write_report("failed", [], {"type": exc.__class__.__name__, "message": str(exc)})
        raise


run()
