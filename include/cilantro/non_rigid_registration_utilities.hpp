#pragma once

#include <Eigen/Sparse>
#include <cilantro/space_transformations.hpp>
#include <cilantro/nearest_neighbors.hpp>
#include <cilantro/correspondence.hpp>
#include <cilantro/common_pair_evaluators.hpp>

#include <iostream>


namespace cilantro {
    template <typename ScalarT>
    inline ScalarT sqrtHuberLoss(ScalarT x, ScalarT delta = (ScalarT)1.0) {
        const ScalarT x_abs = std::abs(x);
        if (x_abs > delta) {
            return std::sqrt(delta*(x_abs - (ScalarT)(0.5)*delta));
        } else {
            return std::sqrt((ScalarT)(0.5))*x_abs;
        }
    }

    template <typename ScalarT>
    inline ScalarT sqrtHuberLossDerivative(ScalarT x, ScalarT delta = (ScalarT)1.0) {
        const ScalarT x_abs = std::abs(x);
        if (x < (ScalarT)0.0) {
            if (x_abs > delta) {
                return -delta/((ScalarT)(2.0)*std::sqrt(delta*(x_abs - (ScalarT)(0.5)*delta)));
            } else {
                return -std::sqrt((ScalarT)(0.5));
            }
        } else {
            if (x_abs > delta) {
                return delta/((ScalarT)(2.0)*std::sqrt(delta*(x_abs - (ScalarT)(0.5)*delta)));
            } else {
                return std::sqrt((ScalarT)0.5);
            }
        }
    }

    template <typename ScalarT>
    void computeRotationTerms(ScalarT a, ScalarT b, ScalarT c,
                              Eigen::Matrix<ScalarT,3,3> &rot_coeffs,
                              Eigen::Matrix<ScalarT,3,3> &d_rot_coeffs_da,
                              Eigen::Matrix<ScalarT,3,3> &d_rot_coeffs_db,
                              Eigen::Matrix<ScalarT,3,3> &d_rot_coeffs_dc)
    {
        const ScalarT sina = std::sin(a);
        const ScalarT cosa = std::cos(a);
        const ScalarT sinb = std::sin(b);
        const ScalarT cosb = std::cos(b);
        const ScalarT sinc = std::sin(c);
        const ScalarT cosc = std::cos(c);

        rot_coeffs(0,0) = cosc*cosb;
        rot_coeffs(1,0) = -sinc*cosa + cosc*sinb*sina;
        rot_coeffs(2,0) = sinc*sina + cosc*sinb*cosa;
        rot_coeffs(0,1) = sinc*cosb;
        rot_coeffs(1,1) = cosc*cosa + sinc*sinb*sina;
        rot_coeffs(2,1) = -cosc*sina + sinc*sinb*cosa;
        rot_coeffs(0,2) = -sinb;
        rot_coeffs(1,2) = cosb*sina;
        rot_coeffs(2,2) = cosb*cosa;

        d_rot_coeffs_da(0,0) = (ScalarT)0.0;
        d_rot_coeffs_da(1,0) = sinc*sina + cosc*sinb*cosa;
        d_rot_coeffs_da(2,0) = sinc*cosa - cosc*sinb*sina;
        d_rot_coeffs_da(0,1) = (ScalarT)0.0;
        d_rot_coeffs_da(1,1) = -cosc*sina + sinc*sinb*cosa;
        d_rot_coeffs_da(2,1) = -cosc*cosa - sinc*sinb*sina;
        d_rot_coeffs_da(0,2) = (ScalarT)0.0;
        d_rot_coeffs_da(1,2) = cosb*cosa;
        d_rot_coeffs_da(2,2) = -cosb*sina;

        d_rot_coeffs_db(0,0) = -cosc*sinb;
        d_rot_coeffs_db(1,0) = cosc*cosb*sina;
        d_rot_coeffs_db(2,0) = cosc*cosb*cosa;
        d_rot_coeffs_db(0,1) = -sinc*sinb;
        d_rot_coeffs_db(1,1) = sinc*cosb*sina;
        d_rot_coeffs_db(2,1) = sinc*cosb*cosa;
        d_rot_coeffs_db(0,2) = -cosb;
        d_rot_coeffs_db(1,2) = -sinb*sina;
        d_rot_coeffs_db(2,2) = -sinb*cosa;

        d_rot_coeffs_dc(0,0) = -sinc*cosb;
        d_rot_coeffs_dc(1,0) = -cosc*cosa - sinc*sinb*sina;
        d_rot_coeffs_dc(2,0) = cosc*sina - sinc*sinb*cosa;
        d_rot_coeffs_dc(0,1) = cosc*cosb;
        d_rot_coeffs_dc(1,1) = -sinc*cosa + cosc*sinb*sina;
        d_rot_coeffs_dc(2,1) = sinc*sina + cosc*sinb*cosa;
        d_rot_coeffs_dc(0,2) = (ScalarT)0.0;
        d_rot_coeffs_dc(1,2) = (ScalarT)0.0;
        d_rot_coeffs_dc(2,2) = (ScalarT)0.0;
    }

    template <typename ScalarT, class PointCorrWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>, class PlaneCorrWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>, class RegWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>>
    bool estimateDenseWarpFieldCombinedMetric3(const ConstVectorSetMatrixMap<ScalarT,3> &dst_p,
                                               const ConstVectorSetMatrixMap<ScalarT,3> &dst_n,
                                               const ConstVectorSetMatrixMap<ScalarT,3> &src_p,
                                               const CorrespondenceSet<typename PointCorrWeightEvaluatorT::InputScalar> &point_to_point_correspondences,
                                               ScalarT point_to_point_weight,
                                               const CorrespondenceSet<typename PlaneCorrWeightEvaluatorT::InputScalar> &point_to_plane_correspondences,
                                               ScalarT point_to_plane_weight,
                                               const std::vector<NeighborSet<typename RegWeightEvaluatorT::InputScalar>> &regularization_neighborhoods,
                                               ScalarT regularization_weight,
                                               RigidTransformationSet<ScalarT,3> &transforms,
                                               ScalarT huber_boundary = (ScalarT)(1e-4),
                                               size_t max_gn_iter = 10,
                                               ScalarT gn_conv_tol = (ScalarT)1e-5,
                                               size_t max_cg_iter = 1000,
                                               ScalarT cg_conv_tol = (ScalarT)1e-5,
                                               const PointCorrWeightEvaluatorT &point_corr_evaluator = PointCorrWeightEvaluatorT(),
                                               const PlaneCorrWeightEvaluatorT &plane_corr_evaluator = PlaneCorrWeightEvaluatorT(),
                                               const RegWeightEvaluatorT &reg_evaluator = RegWeightEvaluatorT())
    {
        const bool has_point_to_point_terms = !point_to_point_correspondences.empty() && (point_to_point_weight > (ScalarT)0.0);
        const bool has_point_to_plane_terms = !point_to_plane_correspondences.empty() && (point_to_plane_weight > (ScalarT)0.0);

        if ((!has_point_to_point_terms && !has_point_to_plane_terms) ||
            (has_point_to_plane_terms && dst_p.cols() != dst_n.cols()))
        {
            transforms.resize(src_p.cols());
            transforms.setIdentity();
            return false;
        }

        // Get regularization equation count and indices
        std::vector<size_t> reg_eq_ind(regularization_neighborhoods.size());
        size_t num_reg_arcs = 0;
        if (!regularization_neighborhoods.empty()) {
            reg_eq_ind[0] = 0;
            num_reg_arcs = std::max((size_t)0, regularization_neighborhoods[0].size() - 1);
        }
        for (size_t i = 1; i < regularization_neighborhoods.size(); i++) {
            reg_eq_ind[i] = reg_eq_ind[i-1] + 6*std::max((size_t)0, regularization_neighborhoods[i-1].size() - 1);
            num_reg_arcs += std::max((size_t)0, regularization_neighborhoods[i].size() - 1);
        }

        // Compute number of equations and unknowns
        const size_t num_unknowns = 6*src_p.cols();
        const size_t num_point_to_point_equations = 3*has_point_to_point_terms*point_to_point_correspondences.size();
        const size_t num_point_to_plane_equations = has_point_to_plane_terms*point_to_plane_correspondences.size();
        const size_t num_data_term_equations = num_point_to_point_equations + num_point_to_plane_equations;
        const size_t num_regularization_equations = 6*num_reg_arcs;
        const size_t num_equations = num_data_term_equations + num_regularization_equations;
        const size_t num_non_zeros = 6*num_data_term_equations + 2*num_regularization_equations;

        // Jacobian
        Eigen::SparseMatrix<ScalarT> At(num_unknowns, num_equations);
        At.reserve(num_non_zeros);
        // Values
        ScalarT * const values = At.valuePtr();
        // Outer pointers
        typename Eigen::SparseMatrix<ScalarT>::StorageIndex * const outer_ptr = At.outerIndexPtr();
#pragma omp parallel
        {
#pragma omp for nowait
            for (size_t i = 0; i < num_data_term_equations + 1; i++) {
                outer_ptr[i] = 6*i;
            }
#pragma omp for nowait
            for (size_t i = 1; i < num_regularization_equations + 1; i++) {
                outer_ptr[num_data_term_equations + i] = 6*num_data_term_equations + 2*i;
            }
        }
        // Inner indices
        typename Eigen::SparseMatrix<ScalarT>::StorageIndex * const inner_ind = At.innerIndexPtr();

        // Vector of (negative) residuals
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> b(num_equations);

        // Vector of unknowns (Euler angles and translation offsets per point)
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> tforms_vec(Eigen::Matrix<ScalarT,Eigen::Dynamic,1>::Zero(num_unknowns, 1));

        // Conjugate Gradient solver
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::IncompleteCholesky<ScalarT,Eigen::Lower|Eigen::Upper>> solver;
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::IdentityPreconditioner> solver;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::DiagonalPreconditioner<ScalarT>> solver;
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,BlockDiagonalPreconditioner<ScalarT,6>> solver;
        solver.setMaxIterations(max_cg_iter);
        solver.setTolerance(cg_conv_tol);

        Eigen::SparseMatrix<ScalarT> AtA;
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> Atb;

        // Parameters
        const ScalarT point_to_point_weight_sqrt = std::sqrt(point_to_point_weight);
        const ScalarT point_to_plane_weight_sqrt = std::sqrt(point_to_plane_weight);
        const ScalarT regularization_weight_sqrt = std::sqrt(regularization_weight);
        const ScalarT gn_conv_tol_sq = gn_conv_tol*gn_conv_tol;

        // Temporaries
        Eigen::Matrix<ScalarT,3,3> rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc;
        Eigen::Matrix<ScalarT,3,1> trans_s, d_rot_da_s, d_rot_db_s, d_rot_dc_s;
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> delta;
        ScalarT weight, diff, d_sqrt_huber_loss, curr_delta_sq, max_delta_sq;
        size_t eq_ind, nz_ind;

        bool has_converged = false;
        size_t iter = 0;
        while (iter < max_gn_iter) {
#pragma omp parallel shared (At, b) private (eq_ind, nz_ind, weight, diff, d_sqrt_huber_loss, rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc, trans_s, d_rot_da_s, d_rot_db_s, d_rot_dc_s)
            {
                // Data term
                if (has_point_to_point_terms) {
#pragma omp for nowait
                    for (size_t i = 0; i < point_to_point_correspondences.size(); i++) {
                        const auto& corr = point_to_point_correspondences[i];
                        const auto d = dst_p.col(corr.indexInFirst);
                        const auto s = src_p.col(corr.indexInSecond);
                        const size_t offset = 6*corr.indexInSecond;
                        weight = point_to_point_weight_sqrt*std::sqrt(point_corr_evaluator(corr.indexInFirst, corr.indexInSecond, corr.value));

                        computeRotationTerms(tforms_vec[offset], tforms_vec[offset + 1], tforms_vec[offset + 2], rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc);
                        const auto trans_coeffs = tforms_vec.template segment<3>(offset + 3);

                        trans_s.noalias() = rot_coeffs.transpose()*s + trans_coeffs - d;
                        d_rot_da_s.noalias() = d_rot_coeffs_da.transpose()*s;
                        d_rot_db_s.noalias() = d_rot_coeffs_db.transpose()*s;
                        d_rot_dc_s.noalias() = d_rot_coeffs_dc.transpose()*s;

                        eq_ind = 3*i;
                        nz_ind = 18*i;

                        values[nz_ind] = d_rot_da_s[0]*weight;
                        inner_ind[nz_ind++] = offset;
                        values[nz_ind] = d_rot_db_s[0]*weight;
                        inner_ind[nz_ind++] = offset + 1;
                        values[nz_ind] = d_rot_dc_s[0]*weight;
                        inner_ind[nz_ind++] = offset + 2;
                        values[nz_ind] = weight;
                        inner_ind[nz_ind++] = offset + 3;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 4;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 5;
                        b[eq_ind++] = -(trans_s[0])*weight;

                        values[nz_ind] = d_rot_da_s[1]*weight;
                        inner_ind[nz_ind++] = offset;
                        values[nz_ind] = d_rot_db_s[1]*weight;
                        inner_ind[nz_ind++] = offset + 1;
                        values[nz_ind] = d_rot_dc_s[1]*weight;
                        inner_ind[nz_ind++] = offset + 2;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 3;
                        values[nz_ind] = weight;
                        inner_ind[nz_ind++] = offset + 4;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 5;
                        b[eq_ind++] = -(trans_s[1])*weight;

                        values[nz_ind] = d_rot_da_s[2]*weight;
                        inner_ind[nz_ind++] = offset;
                        values[nz_ind] = d_rot_db_s[2]*weight;
                        inner_ind[nz_ind++] = offset + 1;
                        values[nz_ind] = d_rot_dc_s[2]*weight;
                        inner_ind[nz_ind++] = offset + 2;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 3;
                        values[nz_ind] = (ScalarT)0.0;
                        inner_ind[nz_ind++] = offset + 4;
                        values[nz_ind] = weight;
                        inner_ind[nz_ind++] = offset + 5;
                        b[eq_ind++] = -(trans_s[2])*weight;
                    }
                }

                if (has_point_to_plane_terms) {
#pragma omp for nowait
                    for (size_t i = 0; i < point_to_plane_correspondences.size(); i++) {
                        const auto& corr = point_to_plane_correspondences[i];
                        const auto d = dst_p.col(corr.indexInFirst);
                        const auto n = dst_n.col(corr.indexInFirst);
                        const auto s = src_p.col(corr.indexInSecond);
                        const size_t offset = 6*corr.indexInSecond;
                        weight = point_to_plane_weight_sqrt*std::sqrt(plane_corr_evaluator(corr.indexInFirst, corr.indexInSecond, corr.value));

                        computeRotationTerms(tforms_vec[offset], tforms_vec[offset + 1], tforms_vec[offset + 2], rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc);
                        const auto trans_coeffs = tforms_vec.template segment<3>(offset + 3);

                        trans_s.noalias() = rot_coeffs.transpose()*s + trans_coeffs - d;
                        d_rot_da_s.noalias() = d_rot_coeffs_da.transpose()*s;
                        d_rot_db_s.noalias() = d_rot_coeffs_db.transpose()*s;
                        d_rot_dc_s.noalias() = d_rot_coeffs_dc.transpose()*s;

                        eq_ind = num_point_to_point_equations + i;
                        nz_ind = 6*num_point_to_point_equations + 6*i;

                        values[nz_ind] = (n.dot(d_rot_da_s))*weight;
                        inner_ind[nz_ind++] = offset;
                        values[nz_ind] = (n.dot(d_rot_db_s))*weight;
                        inner_ind[nz_ind++] = offset + 1;
                        values[nz_ind] = (n.dot(d_rot_dc_s))*weight;
                        inner_ind[nz_ind++] = offset + 2;
                        values[nz_ind] = n[0]*weight;
                        inner_ind[nz_ind++] = offset + 3;
                        values[nz_ind] = n[1]*weight;
                        inner_ind[nz_ind++] = offset + 4;
                        values[nz_ind] = n[2]*weight;
                        inner_ind[nz_ind++] = offset + 5;
                        b[eq_ind++] = -(n.dot(trans_s))*weight;
                    }
                }

                // Regularization
#pragma omp for nowait
                for (size_t i = 0; i < regularization_neighborhoods.size(); i++) {
                    eq_ind = num_data_term_equations + reg_eq_ind[i];
                    nz_ind = 6*num_data_term_equations + 2*reg_eq_ind[i];
                    const auto& neighbors = regularization_neighborhoods[i];

                    for (size_t j = 1; j < neighbors.size(); j++) {
                        size_t s_offset = 6*neighbors[0].index;
                        size_t n_offset = 6*neighbors[j].index;
                        weight = regularization_weight_sqrt*std::sqrt(reg_evaluator(neighbors[0].index, neighbors[j].index, neighbors[j].value));

                        if (n_offset < s_offset) std::swap(s_offset, n_offset);

                        diff = tforms_vec[s_offset + 0] - tforms_vec[n_offset + 0];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 1] - tforms_vec[n_offset + 1];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 1;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 1;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 2] - tforms_vec[n_offset + 2];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 2;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 2;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 3] - tforms_vec[n_offset + 3];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 3;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 3;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 4] - tforms_vec[n_offset + 4];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 4;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 4;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 5] - tforms_vec[n_offset + 5];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 5;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 5;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);
                    }
                }
            }

            // Solve linear system using CG
            AtA = At*At.transpose();
            Atb.noalias() = At*b;

//            solver.compute(AtA);
            if (iter == 0) solver.analyzePattern(AtA);
            solver.factorize(AtA);
            delta = solver.solve(Atb);
            tforms_vec += delta;

            iter++;

            // Check for convergence
            max_delta_sq = (ScalarT)0.0;
#pragma omp parallel for private (curr_delta_sq) reduction (max: max_delta_sq)
            for (size_t i = 0; i < src_p.cols(); i++) {
                curr_delta_sq = delta.template segment<6>(6*i).squaredNorm();
                if (curr_delta_sq > max_delta_sq) max_delta_sq = curr_delta_sq;
            }

//            std::cout << iter << ": " << std::sqrt(max_delta_sq) << std::endl;

            if (max_delta_sq < gn_conv_tol_sq) {
                has_converged = true;
                break;
            }
        }

        // Convert to output format
        transforms.resize(src_p.cols());
#pragma omp parallel for
        for (size_t i = 0; i < transforms.size(); i++) {
            transforms[i].linear().noalias() = (Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 2],Eigen::Matrix<ScalarT,3,1>::UnitZ()) *
                                                Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 1],Eigen::Matrix<ScalarT,3,1>::UnitY()) *
                                                Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 0],Eigen::Matrix<ScalarT,3,1>::UnitX())).matrix();
            transforms[i].linear() = transforms[i].rotation();
            transforms[i].translation() = tforms_vec.template segment<3>(6*i + 3);
        }

        return has_converged;
    }

    template <typename ScalarT, class PointCorrWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>, class PlaneCorrWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>, class ControlWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>, class RegWeightEvaluatorT = UnityWeightEvaluator<ScalarT,ScalarT>>
    bool estimateSparseWarpFieldCombinedMetric3(const ConstVectorSetMatrixMap<ScalarT,3> &dst_p,
                                                const ConstVectorSetMatrixMap<ScalarT,3> &dst_n,
                                                const ConstVectorSetMatrixMap<ScalarT,3> &src_p,
                                                const CorrespondenceSet<typename PointCorrWeightEvaluatorT::InputScalar> &point_to_point_correspondences,
                                                ScalarT point_to_point_weight,
                                                const CorrespondenceSet<typename PlaneCorrWeightEvaluatorT::InputScalar> &point_to_plane_correspondences,
                                                ScalarT point_to_plane_weight,
                                                const std::vector<NeighborSet<typename ControlWeightEvaluatorT::InputScalar>> &src_to_ctrl_neighborhoods,
                                                size_t num_ctrl_points,
                                                const std::vector<NeighborSet<typename RegWeightEvaluatorT::InputScalar>> &regularization_neighborhoods,
                                                ScalarT regularization_weight,
                                                RigidTransformationSet<ScalarT,3> &transforms,
                                                ScalarT huber_boundary = (ScalarT)(1e-6),
                                                size_t max_gn_iter = 10,
                                                ScalarT gn_conv_tol = (ScalarT)1e-5,
                                                size_t max_cg_iter = 1000,
                                                ScalarT cg_conv_tol = (ScalarT)1e-5,
                                                const PointCorrWeightEvaluatorT &point_corr_evaluator = PointCorrWeightEvaluatorT(),
                                                const PlaneCorrWeightEvaluatorT &plane_corr_evaluator = PlaneCorrWeightEvaluatorT(),
                                                const ControlWeightEvaluatorT &control_evaluator = ControlWeightEvaluatorT(),
                                                const RegWeightEvaluatorT &reg_evaluator = RegWeightEvaluatorT())
    {
        const bool has_point_to_point_terms = !point_to_point_correspondences.empty() && (point_to_point_weight > (ScalarT)0.0);
        const bool has_point_to_plane_terms = !point_to_plane_correspondences.empty() && (point_to_plane_weight > (ScalarT)0.0);

        if (src_to_ctrl_neighborhoods.size() != src_p.cols() ||
            (!has_point_to_point_terms && !has_point_to_plane_terms) ||
            (has_point_to_plane_terms && dst_p.cols() != dst_n.cols()))
        {
            transforms.resize(num_ctrl_points);
            transforms.setIdentity();
            return false;
        }

        // Sort control nodes by index and compute total weight
        std::vector<NeighborSet<ScalarT>> src_to_ctrl_sorted(src_to_ctrl_neighborhoods.size());
        std::vector<ScalarT> total_weight(src_to_ctrl_sorted.size());
        std::vector<char> has_data_term(src_to_ctrl_neighborhoods.size(), 0);
#pragma omp parallel shared (src_to_ctrl_sorted, total_weight, has_data_term)
        {
            if (has_point_to_point_terms) {
#pragma omp for nowait
                for (size_t i = 0; i < point_to_point_correspondences.size(); i++) {
                    has_data_term[point_to_point_correspondences[i].indexInSecond] = 1;
                }
            }

            if (has_point_to_plane_terms) {
#pragma omp for
                for (size_t i = 0; i < point_to_plane_correspondences.size(); i++) {
                    has_data_term[point_to_plane_correspondences[i].indexInSecond] = 1;
                }
            }

#pragma omp for schedule (dynamic)
            for (size_t i = 0; i < has_data_term.size(); i++) {
                if (has_data_term[i]) {
                    total_weight[i] = (ScalarT)0.0;
                    src_to_ctrl_sorted[i].resize(src_to_ctrl_neighborhoods[i].size());
                    for (size_t j = 0; j < src_to_ctrl_neighborhoods[i].size(); j++) {
                        src_to_ctrl_sorted[i][j].index = src_to_ctrl_neighborhoods[i][j].index;
                        src_to_ctrl_sorted[i][j].value = control_evaluator(i, src_to_ctrl_neighborhoods[i][j].index, src_to_ctrl_neighborhoods[i][j].value);
                        total_weight[i] += src_to_ctrl_sorted[i][j].value;
                    }
                    std::sort(src_to_ctrl_sorted[i].begin(), src_to_ctrl_sorted[i].end(), typename Neighbor<ScalarT>::IndexLessComparator());
                }
            }
        }

        // Get regularization equation count and indices
        std::vector<size_t> reg_eq_ind(regularization_neighborhoods.size());
        size_t num_reg_arcs = 0;
        if (!regularization_neighborhoods.empty()) {
            reg_eq_ind[0] = 0;
            num_reg_arcs = std::max((size_t)0, regularization_neighborhoods[0].size() - 1);
        }
        for (size_t i = 1; i < regularization_neighborhoods.size(); i++) {
            reg_eq_ind[i] = reg_eq_ind[i-1] + 6*std::max((size_t)0, regularization_neighborhoods[i-1].size() - 1);
            num_reg_arcs += std::max((size_t)0, regularization_neighborhoods[i].size() - 1);
        }

        // Compute number of equations and unknowns
        const size_t num_unknowns = 6*num_ctrl_points;
        const size_t num_point_to_point_equations = 3*has_point_to_point_terms*point_to_point_correspondences.size();
        const size_t num_point_to_plane_equations = has_point_to_plane_terms*point_to_plane_correspondences.size();
        const size_t num_data_term_equations = num_point_to_point_equations + num_point_to_plane_equations;
        const size_t num_regularization_equations = 6*num_reg_arcs;
        const size_t num_equations = num_data_term_equations + num_regularization_equations;

        // Jacobian
        Eigen::SparseMatrix<ScalarT> At(num_unknowns, num_equations);
        // Outer pointers
        typename Eigen::SparseMatrix<ScalarT>::StorageIndex * const outer_ptr = At.outerIndexPtr();
        outer_ptr[0] = 0;
        if (has_point_to_point_terms) {
            for (size_t i = 0; i < point_to_point_correspondences.size(); i++) {
                const size_t nnz_per_eq = 6*src_to_ctrl_sorted[point_to_point_correspondences[i].indexInSecond].size();
                outer_ptr[3*i + 1] = outer_ptr[3*i] + nnz_per_eq;
                outer_ptr[3*i + 2] = outer_ptr[3*i] + nnz_per_eq + nnz_per_eq;
                outer_ptr[3*i + 3] = outer_ptr[3*i] + nnz_per_eq + nnz_per_eq+ nnz_per_eq;
            }
        }
        if (has_point_to_plane_terms) {
            for (size_t i = 0; i < point_to_plane_correspondences.size(); i++) {
                outer_ptr[num_point_to_point_equations + i + 1] = outer_ptr[num_point_to_point_equations + i] + 6*src_to_ctrl_sorted[point_to_plane_correspondences[i].indexInSecond].size();
            }
        }
#pragma omp parallel for
        for (size_t i = 1; i < num_regularization_equations + 1; i++) {
            outer_ptr[num_data_term_equations + i] = outer_ptr[num_data_term_equations] + 2*i;
        }
        At.reserve(outer_ptr[num_equations]);
        // Values
        ScalarT * const values = At.valuePtr();
        // Inner indices
        typename Eigen::SparseMatrix<ScalarT>::StorageIndex * const inner_ind = At.innerIndexPtr();

        // Vector of (negative) residuals
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> b(num_equations);

        // Vector of unknowns (Euler angles and translation offsets per control node)
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> tforms_vec(Eigen::Matrix<ScalarT,Eigen::Dynamic,1>::Zero(num_unknowns, 1));

        // Conjugate Gradient solver
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::IncompleteCholesky<ScalarT,Eigen::Lower|Eigen::Upper>> solver;
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::IdentityPreconditioner> solver;
        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,Eigen::DiagonalPreconditioner<ScalarT>> solver;
//        Eigen::ConjugateGradient<Eigen::SparseMatrix<ScalarT>,Eigen::Lower|Eigen::Upper,BlockDiagonalPreconditioner<ScalarT,6>> solver;
        solver.setMaxIterations(max_cg_iter);
        solver.setTolerance(cg_conv_tol);

        Eigen::SparseMatrix<ScalarT> AtA;
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> Atb;

        // Parameters
        const ScalarT point_to_point_weight_sqrt = std::sqrt(point_to_point_weight);
        const ScalarT point_to_plane_weight_sqrt = std::sqrt(point_to_plane_weight);
        const ScalarT regularization_weight_sqrt = std::sqrt(regularization_weight);
        const ScalarT gn_conv_tol_sq = gn_conv_tol*gn_conv_tol;

        // Temporaries
        Eigen::Matrix<ScalarT,3,3> rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc;
        Eigen::Matrix<ScalarT,3,1> trans_s, d_rot_da_s, d_rot_db_s, d_rot_dc_s;
        Eigen::Matrix<ScalarT,3,1> angles_curr, trans_curr;
        Eigen::Matrix<ScalarT,Eigen::Dynamic,1> delta;
        ScalarT weight, corr_weight_sqrt, corr_weight_nrm, diff, d_sqrt_huber_loss, curr_delta_sq, max_delta_sq;
        size_t eq_ind, nz_ind;

        bool has_converged = false;
        size_t iter = 0;
        while (iter < max_gn_iter) {
#pragma omp parallel shared (At, b) private (eq_ind, nz_ind, weight, corr_weight_sqrt, corr_weight_nrm, diff, d_sqrt_huber_loss, angles_curr, trans_curr, rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc, trans_s, d_rot_da_s, d_rot_db_s, d_rot_dc_s)
            {
                // Data term
                if (has_point_to_point_terms) {
#pragma omp for nowait
                    for (size_t i = 0; i < point_to_point_correspondences.size(); i++) {
                        const auto& corr = point_to_point_correspondences[i];
                        const auto& ctrl_neighbors = src_to_ctrl_sorted[corr.indexInSecond];

                        // Compute weighted influence from control nodes
                        angles_curr.setZero();
                        trans_curr.setZero();
                        for (size_t j = 0; j < ctrl_neighbors.size(); j++) {
                            const size_t offset = 6*ctrl_neighbors[j].index;
                            angles_curr.noalias() += ctrl_neighbors[j].value*tforms_vec.template segment<3>(offset);
                            trans_curr.noalias() += ctrl_neighbors[j].value*tforms_vec.template segment<3>(offset + 3);
                        }
                        if (total_weight[corr.indexInSecond] != (ScalarT)0.0) {
                            weight = (ScalarT)(1.0)/total_weight[corr.indexInSecond];
                            angles_curr *= weight;
                            trans_curr *= weight;

                            corr_weight_sqrt = point_to_point_weight_sqrt*std::sqrt(point_corr_evaluator(corr.indexInFirst, corr.indexInSecond, corr.value));
                            corr_weight_nrm = corr_weight_sqrt/total_weight[corr.indexInSecond];
                        } else {
                            corr_weight_sqrt = (ScalarT)0.0;
                            corr_weight_nrm = (ScalarT)0.0;
                        }

                        const auto d = dst_p.col(corr.indexInFirst);
                        const auto s = src_p.col(corr.indexInSecond);

                        computeRotationTerms(angles_curr[0], angles_curr[1], angles_curr[2], rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc);

                        trans_s.noalias() = rot_coeffs.transpose()*s + trans_curr - d;
                        d_rot_da_s.noalias() = d_rot_coeffs_da.transpose()*s;
                        d_rot_db_s.noalias() = d_rot_coeffs_db.transpose()*s;
                        d_rot_dc_s.noalias() = d_rot_coeffs_dc.transpose()*s;

                        eq_ind = 3*i;

                        for (size_t j = 0; j < ctrl_neighbors.size(); j++) {
                            const size_t offset = 6*ctrl_neighbors[j].index;
                            weight = corr_weight_nrm*ctrl_neighbors[j].value;

                            nz_ind = outer_ptr[eq_ind] + 6*j;
                            values[nz_ind] = d_rot_da_s[0]*weight;
                            inner_ind[nz_ind++] = offset;
                            values[nz_ind] = d_rot_db_s[0]*weight;
                            inner_ind[nz_ind++] = offset + 1;
                            values[nz_ind] = d_rot_dc_s[0]*weight;
                            inner_ind[nz_ind++] = offset + 2;
                            values[nz_ind] = weight;
                            inner_ind[nz_ind++] = offset + 3;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 4;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 5;

                            nz_ind = outer_ptr[eq_ind + 1] + 6*j;
                            values[nz_ind] = d_rot_da_s[1]*weight;
                            inner_ind[nz_ind++] = offset;
                            values[nz_ind] = d_rot_db_s[1]*weight;
                            inner_ind[nz_ind++] = offset + 1;
                            values[nz_ind] = d_rot_dc_s[1]*weight;
                            inner_ind[nz_ind++] = offset + 2;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 3;
                            values[nz_ind] = weight;
                            inner_ind[nz_ind++] = offset + 4;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 5;

                            nz_ind = outer_ptr[eq_ind + 2] + 6*j;
                            values[nz_ind] = d_rot_da_s[2]*weight;
                            inner_ind[nz_ind++] = offset;
                            values[nz_ind] = d_rot_db_s[2]*weight;
                            inner_ind[nz_ind++] = offset + 1;
                            values[nz_ind] = d_rot_dc_s[2]*weight;
                            inner_ind[nz_ind++] = offset + 2;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 3;
                            values[nz_ind] = (ScalarT)0.0;
                            inner_ind[nz_ind++] = offset + 4;
                            values[nz_ind] = weight;
                            inner_ind[nz_ind++] = offset + 5;
                        }

                        b[eq_ind] = -(trans_s[0])*corr_weight_sqrt;
                        b[eq_ind + 1] = -(trans_s[1])*corr_weight_sqrt;
                        b[eq_ind + 2] = -(trans_s[2])*corr_weight_sqrt;
                    }
                }

                if (has_point_to_plane_terms) {
#pragma omp for nowait
                    for (size_t i = 0; i < point_to_plane_correspondences.size(); i++) {
                        const auto& corr = point_to_plane_correspondences[i];
                        const auto& ctrl_neighbors = src_to_ctrl_sorted[corr.indexInSecond];

                        // Compute weighted influence from control nodes
                        angles_curr.setZero();
                        trans_curr.setZero();
                        for (size_t j = 0; j < ctrl_neighbors.size(); j++) {
                            const size_t offset = 6*ctrl_neighbors[j].index;
                            angles_curr.noalias() += ctrl_neighbors[j].value*tforms_vec.template segment<3>(offset);
                            trans_curr.noalias() += ctrl_neighbors[j].value*tforms_vec.template segment<3>(offset + 3);
                        }
                        if (total_weight[corr.indexInSecond] != (ScalarT)0.0) {
                            weight = (ScalarT)(1.0)/total_weight[corr.indexInSecond];
                            angles_curr *= weight;
                            trans_curr *= weight;

                            corr_weight_sqrt = point_to_plane_weight_sqrt*std::sqrt(plane_corr_evaluator(corr.indexInFirst, corr.indexInSecond, corr.value));
                            corr_weight_nrm = corr_weight_sqrt/total_weight[corr.indexInSecond];
                        } else {
                            corr_weight_sqrt = (ScalarT)0.0;
                            corr_weight_nrm = (ScalarT)0.0;
                        }

                        const auto d = dst_p.col(corr.indexInFirst);
                        const auto n = dst_n.col(corr.indexInFirst);
                        const auto s = src_p.col(corr.indexInSecond);

                        computeRotationTerms(angles_curr[0], angles_curr[1], angles_curr[2], rot_coeffs, d_rot_coeffs_da, d_rot_coeffs_db, d_rot_coeffs_dc);

                        trans_s.noalias() = rot_coeffs.transpose()*s + trans_curr - d;
                        d_rot_da_s.noalias() = d_rot_coeffs_da.transpose()*s;
                        d_rot_db_s.noalias() = d_rot_coeffs_db.transpose()*s;
                        d_rot_dc_s.noalias() = d_rot_coeffs_dc.transpose()*s;

                        eq_ind = num_point_to_point_equations + i;

                        for (size_t j = 0; j < ctrl_neighbors.size(); j++) {
                            const size_t offset = 6*ctrl_neighbors[j].index;
                            weight = corr_weight_nrm*ctrl_neighbors[j].value;

                            // Point to plane
                            nz_ind = outer_ptr[eq_ind] + 6*j;
                            values[nz_ind] = (n.dot(d_rot_da_s))*weight;
                            inner_ind[nz_ind++] = offset;
                            values[nz_ind] = (n.dot(d_rot_db_s))*weight;
                            inner_ind[nz_ind++] = offset + 1;
                            values[nz_ind] = (n.dot(d_rot_dc_s))*weight;
                            inner_ind[nz_ind++] = offset + 2;
                            values[nz_ind] = n[0]*weight;
                            inner_ind[nz_ind++] = offset + 3;
                            values[nz_ind] = n[1]*weight;
                            inner_ind[nz_ind++] = offset + 4;
                            values[nz_ind] = n[2]*weight;
                            inner_ind[nz_ind++] = offset + 5;
                        }

                        b[eq_ind] = -(n.dot(trans_s))*corr_weight_sqrt;
                    }
                }

                // Regularization
#pragma omp for nowait
                for (size_t i = 0; i < regularization_neighborhoods.size(); i++) {
                    eq_ind = num_data_term_equations + reg_eq_ind[i];
                    nz_ind = outer_ptr[num_data_term_equations] + 2*reg_eq_ind[i];
                    const auto& neighbors = regularization_neighborhoods[i];

                    for (size_t j = 1; j < neighbors.size(); j++) {
                        size_t s_offset = 6*neighbors[0].index;
                        size_t n_offset = 6*neighbors[j].index;
                        weight = regularization_weight_sqrt*std::sqrt(reg_evaluator(neighbors[0].index, neighbors[j].index, neighbors[j].value));

                        if (n_offset < s_offset) std::swap(s_offset, n_offset);

                        diff = tforms_vec[s_offset + 0] - tforms_vec[n_offset + 0];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 1] - tforms_vec[n_offset + 1];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 1;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 1;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 2] - tforms_vec[n_offset + 2];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 2;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 2;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 3] - tforms_vec[n_offset + 3];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 3;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 3;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 4] - tforms_vec[n_offset + 4];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 4;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 4;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);

                        diff = tforms_vec[s_offset + 5] - tforms_vec[n_offset + 5];
                        d_sqrt_huber_loss = weight*sqrtHuberLossDerivative<ScalarT>(diff, huber_boundary);
                        values[nz_ind] = d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = s_offset + 5;
                        values[nz_ind] = -d_sqrt_huber_loss;
                        inner_ind[nz_ind++] = n_offset + 5;
                        b[eq_ind++] = -weight*sqrtHuberLoss<ScalarT>(diff, huber_boundary);
                    }
                }
            }

            // Solve linear system using CG
            AtA = At*At.transpose();
            Atb.noalias() = At*b;

//            solver.compute(AtA);
            if (iter == 0) solver.analyzePattern(AtA);
            solver.factorize(AtA);
            delta = solver.solve(Atb);
            tforms_vec += delta;

            iter++;

            // Check for convergence
            max_delta_sq = (ScalarT)0.0;
#pragma omp parallel for private (curr_delta_sq) reduction (max: max_delta_sq)
            for (size_t i = 0; i < num_ctrl_points; i++) {
                curr_delta_sq = delta.template segment<6>(6*i).squaredNorm();
                if (curr_delta_sq > max_delta_sq) max_delta_sq = curr_delta_sq;
            }

//            std::cout << iter << ": " << std::sqrt(max_delta_sq) << std::endl;

            if (max_delta_sq < gn_conv_tol_sq) {
                has_converged = true;
                break;
            }

        }

        // Convert to output format
        transforms.resize(num_ctrl_points);
#pragma omp parallel for
        for (size_t i = 0; i < transforms.size(); i++) {
            transforms[i].linear().noalias() = (Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 2],Eigen::Matrix<ScalarT,3,1>::UnitZ()) *
                                                Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 1],Eigen::Matrix<ScalarT,3,1>::UnitY()) *
                                                Eigen::AngleAxis<ScalarT>(tforms_vec[6*i + 0],Eigen::Matrix<ScalarT,3,1>::UnitX())).matrix();
            transforms[i].linear() = transforms[i].rotation();
            transforms[i].translation() = tforms_vec.template segment<3>(6*i + 3);
        }

        return has_converged;
    }
}
