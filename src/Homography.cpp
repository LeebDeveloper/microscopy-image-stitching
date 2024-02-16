#include "Homography.h"
#include "SVD.h"
#include <vector>

double** homography2d(double** points1, double** points2, int s) {
    // Compute H using normalized DLT
    auto** H = new double* [3];
    for (int i = 0; i < 3; i++)
        H[i] = new double[3];

    std::vector<double> A(s * 2 * 8);
    std::vector<double> B(s * 2 * 8);
    std::vector<double> U(s * 2 * 8);
    std::vector<double> V(8 * 8);
    std::vector<double> singular_value(8);
    std::vector<double> dummy_array(8);
    std::vector<double> X(8);
    double tolerance = 0.0;

    double a1, b1, a2, b2;
    for (int i = 0; i < s; i++) {
        a1 = points2[0][i];
        b1 = points2[1][i];
        a2 = points1[0][i];
        b2 = points1[1][i];
        B[i * 2] = a2;
        B[i * 2 + 1] = b2;
        double point[16] = {
                a1, b1, 1, 0,
                0, 0, -a1 * a2, -b1 * a2,
                0, 0, 0, a1,
                b1, 1, -a1 * b2, -b1 * b2 };

        for (int j = 0; j < 16; j++) {
            A[i * 16 + j] = point[j];
        }
    }

    int err = Singular_Value_Decomposition(&A[0], s * 2, 8, &U[0], &singular_value[0], &V[0], &dummy_array[0]);

    if (err < 0) {
        return nullptr;
    } else {
        Singular_Value_Decomposition_Solve(&U[0], &singular_value[0], &V[0], tolerance, s * 2, 8, &B[0], &X[0]);
    }

    for (int i = 0; i < 2; i++)
    {
        H[i][0] = X[i * 3];
        H[i][1] = X[i * 3 + 1];
        H[i][2] = X[i * 3 + 2];
    }

    H[2][0] = X[6];
    H[2][1] = X[7];
    H[2][2] = 1;

    return H;
}//homography2d
