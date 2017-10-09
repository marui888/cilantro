#pragma once

#include <cilantro/convex_polytope.hpp>
#include <cilantro/point_cloud.hpp>

typedef ConvexPolytope<float,float,2> ConvexHull2D;
typedef ConvexPolytope<float,float,3> ConvexHull3D;

class PointCloudHullFlat : public PrincipalComponentAnalysis3D, public ConvexHull2D {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    PointCloudHullFlat(const std::vector<Eigen::Vector3f> &points, bool compute_topology = true, bool simplicial_facets = true, double merge_tol = 0.0);
    PointCloudHullFlat(const PointCloud &cloud, bool compute_topology = true, bool simplicial_facets = true, double merge_tol = 0.0);

    inline const std::vector<Eigen::Vector3f>& getVertices3D() const { return vertices_3d_; }
    inline Eigen::Map<const Eigen::Matrix<float,3,Eigen::Dynamic> > getVertices3DMatrixMap() {
        return Eigen::Map<const Eigen::Matrix<float,3,Eigen::Dynamic> >((float *)vertices_3d_.data(), 3, vertices_3d_.size());
    }

    PointCloudHullFlat& transform(const Eigen::Ref<const Eigen::Matrix3f> &rotation, const Eigen::Ref<const Eigen::Vector3f> &translation);
    PointCloudHullFlat& transform(const Eigen::Ref<const Eigen::Matrix4f> &rigid_transform);

protected:
    std::vector<Eigen::Vector3f> vertices_3d_;
};

class PointCloudHull : public ConvexHull3D {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using ConvexHull3D::getPointSignedDistancesFromFacets;
    using ConvexHull3D::getInteriorPointIndices;
    PointCloudHull(const std::vector<Eigen::Vector3f> &points, bool compute_topology = true, bool simplicial_facets = true, double merge_tol = 0.0);
    PointCloudHull(const PointCloud &cloud, bool compute_topology = true, bool simplicial_facets = true, double merge_tol = 0.0);
    Eigen::MatrixXf getPointSignedDistancesFromFacets(const PointCloud &cloud) const;
    std::vector<size_t> getInteriorPointIndices(const PointCloud &cloud, float offset = 0.0) const;
};
