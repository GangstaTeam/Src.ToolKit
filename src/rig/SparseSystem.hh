#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace tk
{

class SparseMatrix
{
public:
    explicit SparseMatrix(size_t size) : m_rows(size) {}

    void Add(uint32_t row, uint32_t column, double value);
    void Finish();
    void Multiply(const std::vector<double>& x, std::vector<double>& out) const;
    const std::vector<double>& Diagonal() const { return m_diagonal; }
    size_t Size() const { return m_rows.size(); }

private:
    std::vector<std::vector<std::pair<uint32_t, double>>> m_rows;
    std::vector<size_t> m_start;
    std::vector<uint32_t> m_columns;
    std::vector<double> m_values;
    std::vector<double> m_diagonal;
};

bool SolveConjugateGradient(const SparseMatrix& matrix,
    const std::vector<double>& rhs,
    std::vector<double>& x,
    int maxIterations,
    double tolerance);

}
