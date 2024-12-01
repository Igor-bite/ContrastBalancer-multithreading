#include <cuda_runtime.h>
#include <cuda.h>
#include <stdio.h>
using namespace std;

// Cheat sheet -> https://kdm.icm.edu.pl/Tutorials/GPU-intro/introduction.en/#:~:text=The%20maximum%20number%20of%20threads,a%20block%20of%201024%20threads.

#define EXIT_CODE_ERROR 1
#define CUDA_WRAP_ERROR(FUNCTION)                                   \
if (auto err = FUNCTION) {                                          \
    fprintf(stderr, "%s:%d function failed!", __FILE__, __LINE__);  \
    return EXIT_CODE_ERROR;                                         \
}

__global__ void kernel_add(int *c, const int *a, const int *b)
{
    unsigned int x = threadIdx.x;
    c[x] = a[x] + b[x];
}

void printArray(int* a, int size)
{
    printf("{");
    for (size_t i = 0; i < size - 1; i++)
    {
        printf("%i, ", a[i]);
    }
    if (size > 0)
    {
        printf("%i", a[size - 1]);
    }
    printf("}");
}

int main()
{
    int i;

    CUDA_WRAP_ERROR(cudaGetDeviceCount(&i));
    fprintf(stdout, "Device count: %i\n", i);

    cudaDeviceProp prop{};
    CUDA_WRAP_ERROR(cudaGetDeviceProperties(&prop, 0));

    fprintf(stdout, "Name: %s\n", prop.name);
    fprintf(stdout, "major.minor: %i.%i\n\n", prop.major, prop.minor);

    CUDA_WRAP_ERROR(cudaSetDevice(0));

    constexpr int array_size = 5;
    constexpr int byte_size = array_size * sizeof(int);
    int a[array_size] = { 1, 2, 3, 4, 5 };
    int b[array_size] = { 10, 20, 30, 40, 50 };
    int c[array_size];

    int *dev_a, *dev_b, *dev_c;

    CUDA_WRAP_ERROR(cudaMalloc(&dev_a, byte_size));
    CUDA_WRAP_ERROR(cudaMalloc(&dev_b, byte_size));
    CUDA_WRAP_ERROR(cudaMalloc(&dev_c, byte_size));

    CUDA_WRAP_ERROR(cudaMemcpyAsync(dev_a, a, byte_size, cudaMemcpyKind::cudaMemcpyHostToDevice));
    CUDA_WRAP_ERROR(cudaMemcpyAsync(dev_b, b, byte_size, cudaMemcpyKind::cudaMemcpyHostToDevice));

    kernel_add<<<1, 5>>>(dev_c, dev_a, dev_b);
    CUDA_WRAP_ERROR(cudaGetLastError());

    CUDA_WRAP_ERROR(cudaMemcpy(c, dev_c, byte_size, cudaMemcpyKind::cudaMemcpyDeviceToHost));

    printArray(a, array_size);
    printf(" + ");
    printArray(b, array_size);
    printf(" = ");
    printArray(c, array_size);
    printf("\n\n");

    CUDA_WRAP_ERROR(cudaFree(dev_a));
    CUDA_WRAP_ERROR(cudaFree(dev_b));
    CUDA_WRAP_ERROR(cudaFree(dev_c));

    return 0;
}
