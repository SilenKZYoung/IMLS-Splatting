#include <cstdint>
#include <cuda_runtime.h>

namespace cumc
{
  template <typename T>
  struct Vertex
  {
    T x, y, z;

    inline __device__ __host__ T *data_ptr() { return &x; }

    inline __device__ __host__ Vertex<T> operator+(Vertex<T> const &other) const
    {
      return {x + other.x, y + other.y, z + other.z};
    }
    inline __device__ __host__ T dot(Vertex<T> const &other) const
    {
      return x * other.x + y * other.y + z * other.z;
    }

    inline __device__ __host__ Vertex<T> operator-(Vertex<T> const &other) const
    {
      return {x - other.x, y - other.y, z - other.z};
    }

    inline __device__ __host__ Vertex<T> operator*(Vertex<T> const &other) const
    {
      return {x * other.x, y * other.y, z * other.z};
    }

    inline __device__ __host__ Vertex<T> operator*(T const &scalar) const
    {
      return {x * scalar, y * scalar, z * scalar};
    }

    inline __device__ __host__ Vertex<T> &operator+=(Vertex<T> const &other)
    {
      x += other.x;
      y += other.y;
      z += other.z;
      return *this;
    }

    inline __device__ __host__ Vertex<T> &operator-=(Vertex<T> const &other)
    {
      x -= other.x;
      y -= other.y;
      z -= other.z;
      return *this;
    }

    inline __device__ __host__ Vertex<T> &operator*=(T const &scalar)
    {
      x *= scalar;
      y *= scalar;
      z *= scalar;
      return *this;
    }
  };

template <typename T>
struct Feature
{
    static const int SIZE = 8;
    T data[SIZE];

    inline __device__ __host__ T* data_ptr() { return data; }

    inline __device__ __host__ Feature<T> operator+(Feature<T> const &other) const
    {
        Feature<T> result;
        for (int i = 0; i < SIZE; ++i) {
            result.data[i] = data[i] + other.data[i];
        }
        return result;
    }

    inline __device__ __host__ T dot(Feature<T> const &other) const
    {
        T result = 0;
        for (int i = 0; i < SIZE; ++i) {
            result += data[i] * other.data[i];
        }
        return result;
    }

    inline __device__ __host__ Feature<T> operator-(Feature<T> const &other) const
    {
        Feature<T> result;
        for (int i = 0; i < SIZE; ++i) {
            result.data[i] = data[i] - other.data[i];
        }
        return result;
    }

    inline __device__ __host__ Feature<T> operator*(Feature<T> const &other) const
    {
        Feature<T> result;
        for (int i = 0; i < SIZE; ++i) {
            result.data[i] = data[i] * other.data[i];
        }
        return result;
    }

    inline __device__ __host__ Feature<T> operator*(T const &scalar) const
    {
        Feature<T> result;
        for (int i = 0; i < SIZE; ++i) {
            result.data[i] = data[i] * scalar;
        }
        return result;
    }

    inline __device__ __host__ Feature<T>& operator+=(Feature<T> const &other)
    {
        for (int i = 0; i < SIZE; ++i) {
            data[i] += other.data[i];
        }
        return *this;
    }

    inline __device__ __host__ Feature<T>& operator-=(Feature<T> const &other)
    {
        for (int i = 0; i < SIZE; ++i) {
            data[i] -= other.data[i];
        }
        return *this;
    }

    inline __device__ __host__ Feature<T>& operator*=(T const &scalar)
    {
        for (int i = 0; i < SIZE; ++i) {
            data[i] *= scalar;
        }
        return *this;
    }

    inline __device__ void atomicAddWapper(Feature<T> const &other)
    {
        for (int i = 0; i < SIZE; ++i) {
            atomicAdd(&data[i], other.data[i]);
        }
    }

};


  template <typename T>
  struct Triangle
  {
    T i, j, k;
    inline __device__ __host__ T *data_ptr() { return &i; }
  };

  template <typename Scalar, typename IndexType>
  struct CuMC
  {
    IndexType dims[3]{};
    IndexType n_cells{};

    IndexType n_used_cells{0};
    IndexType n_verts{0};
    IndexType n_tris{0};

    // temp storage
    size_t allocated_temp_storage_size{};
    IndexType *__restrict__ temp_storage{}; // used for prefix sum

    size_t allocated_cell_count{};
    IndexType *__restrict__ first_cell_used{}; // cell to used cell index

    //  used cell
    size_t allocated_used_cell_count{};
    IndexType *__restrict__ used_cell_index{};        // used cell to cell index
    IndexType *__restrict__ used_to_first_mc_vert{};  // used cell to mc vertex index
    uint8_t *__restrict__ used_cell_code{};           // used cell to cube code
    IndexType *__restrict__ used_to_first_mc_tri{};   // used cell to mc tri index

    // output
    size_t allocated_vert_count{};
    IndexType *__restrict__ verts_type{}; // verts type
    Vertex<Scalar> *__restrict__ verts{}; // output verts
    Feature<Scalar> *__restrict__ feats{}; // output verts
    size_t allocated_tri_count{};
    IndexType *__restrict__ tris{}; // output triangles

    inline __device__ __host__ IndexType gA(IndexType const x, IndexType const y,
                                            IndexType const z) const
    {
      return z + dims[2] * (y + dims[1] * x);
    }
    inline __device__ __host__ IndexType gX(IndexType const linearizedCellID) const
    {
      return linearizedCellID / (dims[2] * dims[1]);
    }
    inline __device__ __host__ IndexType gY(IndexType const linearizedCellID) const
    {
      return (linearizedCellID / dims[2]) % dims[1];
    }
    inline __device__ __host__ IndexType gZ(IndexType const linearizedCellID) const
    {
      return linearizedCellID % dims[2];
    }

    inline __host__ void resize(IndexType x, IndexType y, IndexType z)
    {
      dims[0] = x;
      dims[1] = y;
      dims[2] = z;
      n_cells = x * y * z;
    }

    __host__ void ensure_temp_storage_size(size_t size);
    __host__ void ensure_cell_storage_size(size_t cell_count);
    __host__ void ensure_used_cell_storage_size(size_t cell_count);
    __host__ void ensure_vert_type_storage_size(size_t vert_count);
    __host__ void ensure_tri_storage_size(size_t tri_count);
    __host__ void ensure_vert_storage_size(size_t vert_count);

    __host__ void forward(Scalar const *sdfsgrid,
                          Feature<Scalar> const *featgrid, 
                          IndexType dimX, IndexType dimY, IndexType dimZ,
                          Scalar isovalue, int device);

    __host__ void backward(Scalar          const *sdfsgrid, 
                           Feature<Scalar> const *featgrid, 
                           Vertex<Scalar>  const *adj_verts,
                           Feature<Scalar> const *adj_feats,
                           Scalar *adj_sdfsgrid,
                           Feature<Scalar> *adj_featgrid,
                           Scalar isovalue,
                           int    device);            
  };

} // namespace cumc
