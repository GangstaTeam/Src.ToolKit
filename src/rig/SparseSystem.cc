#include "rig/SparseSystem.hh"

#include <algorithm>
#include <cmath>

namespace tk
{

namespace
{

double DotProduct(const std::vector<double>& a, const std::vector<double>& b)
{
    double sum = 0.0;

    for (size_t i = 0; i < a.size(); ++i)
    {
        sum += a[i] * b[i];
    }

    return sum;
}

}

void SparseMatrix::Add(uint32_t row, uint32_t column, double value)
{
    m_rows[row].push_back({column, value});
}

void SparseMatrix::Finish()
{
    m_start.assign(1, 0);
    m_columns.clear();
    m_values.clear();
    m_diagonal.assign(m_rows.size(), 0.0);

    for (size_t r = 0; r < m_rows.size(); ++r)
    {
        auto& row = m_rows[r];
        std::sort(row.begin(), row.end());
        size_t rowStart = m_columns.size();

        for (const auto& [column, value] : row)
        {
            if (m_columns.size() > rowStart && m_columns.back() == column)
            {
                m_values.back() += value;
            }
            else
            {
                m_columns.push_back(column);
                m_values.push_back(value);
            }
        }

        for (size_t k = m_start.back(); k < m_columns.size(); ++k)
        {
            if (m_columns[k] == r)
            {
                m_diagonal[r] = m_values[k];
            }
        }

        m_start.push_back(m_columns.size());
        row.clear();
        row.shrink_to_fit();
    }
}

void SparseMatrix::Multiply(const std::vector<double>& x, std::vector<double>& out) const
{
    out.assign(x.size(), 0.0);

    for (size_t r = 0; r + 1 < m_start.size(); ++r)
    {
        double sum = 0.0;

        for (size_t k = m_start[r]; k < m_start[r + 1]; ++k)
        {
            sum += m_values[k] * x[m_columns[k]];
        }

        out[r] = sum;
    }
}

bool SolveConjugateGradient(const SparseMatrix& matrix,
    const std::vector<double>& rhs,
    std::vector<double>& x,
    int maxIterations,
    double tolerance)
{
    size_t n = matrix.Size();
    const std::vector<double>& diagonal = matrix.Diagonal();
    std::vector<double> r(n), z(n), p(n), ap(n);
    matrix.Multiply(x, ap);

    for (size_t i = 0; i < n; ++i)
    {
        r[i] = rhs[i] - ap[i];
    }

    for (size_t i = 0; i < n; ++i)
    {
        z[i] = diagonal[i] > 0.0 ? r[i] / diagonal[i] : r[i];
    }

    p = z;
    double rz = DotProduct(r, z), target = tolerance * tolerance * std::max(DotProduct(rhs, rhs), 1e-30);

    for (int iteration = 0; iteration < maxIterations; ++iteration)
    {
        if (DotProduct(r, r) <= target)
        {
            return true;
        }

        matrix.Multiply(p, ap);
        double alpha = rz / std::max(DotProduct(p, ap), 1e-300);

        for (size_t i = 0; i < n; ++i)
        {
            x[i] += alpha * p[i];
            r[i] -= alpha * ap[i];
        }

        for (size_t i = 0; i < n; ++i)
        {
            z[i] = diagonal[i] > 0.0 ? r[i] / diagonal[i] : r[i];
        }

        double next = DotProduct(r, z);

        for (size_t i = 0; i < n; ++i)
        {
            p[i] = z[i] + (next / rz) * p[i];
        }

        rz = next;
    }

    return DotProduct(r, r) <= target * 100.0;
}

}
