#include "udf.h"

DEFINE_ON_DEMAND(m16_wall_geometry_probe)
{
    Domain *domain = Get_Domain(1);
    Thread *t;
    thread_loop_f(t, domain)
    {
        if (BOUNDARY_FACE_THREAD_P(t) && THREAD_TYPE(t) == THREAD_F_WALL)
        {
            face_t f;
            real area_sum = 0.0;
            real min_xyz[ND_ND] = {1.0e30, 1.0e30, 1.0e30};
            real max_xyz[ND_ND] = {-1.0e30, -1.0e30, -1.0e30};
            real xyz_sum[ND_ND] = {0.0, 0.0, 0.0};
            begin_f_loop(f, t)
            {
                real xyz[ND_ND];
                real area[ND_ND];
                real amag;
                F_CENTROID(xyz, f, t);
                F_AREA(area, f, t);
                amag = NV_MAG(area);
                area_sum += amag;
                xyz_sum[0] += amag * xyz[0];
                xyz_sum[1] += amag * xyz[1];
                xyz_sum[2] += amag * xyz[2];
                if (xyz[0] < min_xyz[0]) min_xyz[0] = xyz[0];
                if (xyz[1] < min_xyz[1]) min_xyz[1] = xyz[1];
                if (xyz[2] < min_xyz[2]) min_xyz[2] = xyz[2];
                if (xyz[0] > max_xyz[0]) max_xyz[0] = xyz[0];
                if (xyz[1] > max_xyz[1]) max_xyz[1] = xyz[1];
                if (xyz[2] > max_xyz[2]) max_xyz[2] = xyz[2];
            }
            end_f_loop(f, t)
            if (area_sum > 0.0)
                Message("M16 wall geom: id=%d name=%s area=%g bbox=(%g,%g,%g)-(%g,%g,%g) centroid=(%g,%g,%g)\n",
                        THREAD_ID(t), THREAD_NAME(t), area_sum,
                        min_xyz[0], min_xyz[1], min_xyz[2],
                        max_xyz[0], max_xyz[1], max_xyz[2],
                        xyz_sum[0] / area_sum, xyz_sum[1] / area_sum,
                        xyz_sum[2] / area_sum);
        }
    }
}
