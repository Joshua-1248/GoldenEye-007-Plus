#include <ultra64.h>
#include <bondtypes.h>

/**
 * Tests whether an infinite line intersects a triangle, writing the hit position and unnormalized
 * face normal into *hit on success. All the math is done as f64/double.
 *
 * @param vertex0 First triangle vertex in local coordinates.
 * @param vertex1 Second triangle vertex in local coordinates.
 * @param vertex2 Third triangle vertex in local coordinates.
 * @param vertexOffset Translation applied to each triangle vertex, moving locally stored
 * geometry into the tested line's coordinate space.
 * @param rayStart Ray origin used to reject intersections behind it.
 * @param linePoint Point on the infinite line used for the plane intersection.
 * @param rayDirection Direction of the line and forward ray test.
 * @param hit Receives the intersection position and face normal on success.
 */
bool intersectRayTriangle(Vertex *vertex0, Vertex *vertex1, Vertex *vertex2, coord3d *vertexOffset, coord3d *rayStart, coord3d *linePoint, coord3d *rayDirection, HitThing *hit)
{
    f64 edge01[3];
    f64 edge12[3];
    f64 edge02[3];
    f64 normalX;
    f64 normalY;
    f64 normalZ;
    f64 planeConstant;
    f64 denominator;
    f64 baryU;
    f64 hitPosition[3];
    f64 relativePosition[3];
    f64 baryV;
    f64 vertexOffset64[3];
    f64 rayDirection64[3];
    f64 linePoint64[3];
    f64 lineParameter;
 
    // Widen the 32-bit coord3d components to f64.
    vertexOffset64[0] = vertexOffset->x;
    vertexOffset64[1] = vertexOffset->y;
    vertexOffset64[2] = vertexOffset->z;
 
    rayDirection64[0] = rayDirection->x;
    rayDirection64[1] = rayDirection->y;
    rayDirection64[2] = rayDirection->z;
 
    linePoint64[0] = linePoint->x;
    linePoint64[1] = linePoint->y;
    linePoint64[2] = linePoint->z;
 
    /**
     * Build two adjacent edges of the triangle. The shared vertex offset cancels when
     * subtracting endpoints, so these vectors can remain in the triangle's local space.
     */
    edge01[0] = vertex1->coord.x - vertex0->coord.x;
    edge12[0] = vertex2->coord.x - vertex1->coord.x;
    edge01[1] = vertex1->coord.y - vertex0->coord.y;
    edge12[1] = vertex2->coord.y - vertex1->coord.y;
    edge01[2] = vertex1->coord.z - vertex0->coord.z;
    edge12[2] = vertex2->coord.z - vertex1->coord.z;

    // Build the direct edge from vertex0 to vertex2 for the barycentric calculations.
    edge02[0] = vertex2->coord.x - vertex0->coord.x;
    edge02[1] = vertex2->coord.y - vertex0->coord.y;
    edge02[2] = vertex2->coord.z - vertex0->coord.z;
 
    // Take the cross product of the triangle edges to obtain its unnormalized face normal.
    normalX = (edge01[1] * edge12[2]) - (edge12[1] * edge01[2]);
    normalY = (edge01[2] * edge12[0]) - (edge12[2] * edge01[0]);
    normalZ = (edge01[0] * edge12[1]) - (edge12[0] * edge01[1]);
 
    /**
     * Place the triangle's plane in the line's coordinate space by translating vertex0.
     * The resulting plane equation is normal's dot product with point vertex0 = planeConstant.
     */
    planeConstant = ((normalX * (vertex0->coord.x + vertexOffset64[0])) + (normalY * (vertex0->coord.y + vertexOffset64[1]))) + (normalZ * (vertex0->coord.z + vertexOffset64[2]));
 
    /**
    * Measure how directly the line points through the plane. A zero dot product means
    * the line runs parallel to the plane and therefore does not intersect.
    */
    denominator = ((normalX * rayDirection64[0]) + (normalY * rayDirection64[1])) + (normalZ * rayDirection64[2]);
 
    if (denominator == 0.0)
    {
        // Line is parallel to the tri's plane, return false for no hit.
        return FALSE;
    }
 
    // Solve the plane equation for the parameter along the infinite line.
    lineParameter = (((planeConstant - (normalX * linePoint64[0])) - (normalY * linePoint64[1])) - (normalZ * linePoint64[2])) / denominator;
 
    /**
     * Evaluate linePoint + rayDirection * lineParameter to get the plane intersection.
     */
    hitPosition[0] = linePoint64[0] + (rayDirection64[0] * lineParameter);
    hitPosition[1] = linePoint64[1] + (rayDirection64[1] * lineParameter);
    hitPosition[2] = linePoint64[2] + (rayDirection64[2] * lineParameter);
 
    /**
     * Express the intersection relative to translated vertex0. This lets the following 
     * calculations determine where the point lies within the triangle.
     */
    relativePosition[0] = hitPosition[0] - (vertex0->coord.x + vertexOffset64[0]);
    relativePosition[1] = hitPosition[1] - (vertex0->coord.y + vertexOffset64[1]);
    relativePosition[2] = hitPosition[2] - (vertex0->coord.z + vertexOffset64[2]);
 
    // Solve the first barycentric weight using the triangle's XY projection.
    denominator = (edge01[1] * edge12[0]) - (edge01[0] * edge12[1]);
 
    if (denominator != 0.0)
    {
        baryU = ((relativePosition[0] * edge01[1]) - (relativePosition[1] * edge01[0])) / denominator;
    }
    else
    {
        // The XY projection is degenerate, so try the YZ projection instead.
        denominator = (edge01[2] * edge12[1]) - (edge01[1] * edge12[2]);
 
        if (denominator != 0.0)
        {
            baryU = ((relativePosition[1] * edge01[2]) - (relativePosition[2] * edge01[1])) / denominator;
        }
        else
        {
            // The YZ projection is also degenerate, so use the XZ projection.
            denominator = (edge01[0] * edge12[2]) - (edge01[2] * edge12[0]);
            baryU = ((relativePosition[2] * edge01[0]) - (relativePosition[0] * edge01[2])) / denominator;
        }
    }
 
    // Solve the second barycentric weight using a nonzero component of edge01.
    if (edge01[0] != 0.0)
    {
        baryV = (relativePosition[0] - (baryU * edge02[0])) / edge01[0];
    }
    else if (edge01[1] != 0.0)
    {
        baryV = (relativePosition[1] - (baryU * edge02[1])) / edge01[1];
    }
    else
    {
        baryV = (relativePosition[2] - (baryU * edge02[2])) / edge01[2];
    }
 
    /** 
     * Both weights must be nonnegative and their sum must not exceed one. This is the
     * barycentric test for whether the plane intersection is inside or on the triangle.
     */
    if (((baryV >= 0.0) && (baryU >= 0.0)) && ((baryV + baryU) <= 1.0))
    {
        /** 
         * The plane calculation used an infinite line. This dot product verifies that the
         * intersection lies at or in front of rayStart rather than behind the ray.
         */
        if ((((rayDirection64[0] * (hitPosition[0] - rayStart->x)) + (rayDirection64[1] * (hitPosition[1] - rayStart->y))) + (rayDirection64[2] * (hitPosition[2] - rayStart->z))) >= 0.0)
        {
            hit->hitpos.x = hitPosition[0];
            hit->hitpos.y = hitPosition[1];
            hit->hitpos.z = hitPosition[2];
 
            hit->normal.x = normalX;
            hit->normal.y = normalY;
            hit->normal.z = normalZ;
 
            hit->texturenum = 0;
 
            return TRUE;
        }
 
        // The infinite line hit the triangle, but the intersection was behind rayStart.
        return FALSE;
    }
 
    // The line reached the triangle's plane outside the triangle's three edges.
    return FALSE;
}
