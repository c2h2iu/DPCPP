#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 8;


class syclNode{

};


class syclTask{
public:
    template <typename... Ts>
    cudaTask& precede(Ts&&... tasks);

    template <typename... Ts>
    cudaTask& succeed(Ts&&... tasks);
};


class syclFlow{
public:
    
    template <typename R, typename C>
    syclTask parallel_for(R& range, C&& callable);

    template <typename T>
    syclTask memcpy(T* tgt, const T* src, size_t num);

    void run();

    syclFlow(queue& q){
        deviceQueue = q;    
        std::cout << "constructor\n"; 
    }


private:
    queue deviceQueue;

};


int main(){
    syclFlow sf;

    queue q;

    int* array_host = static_cast<int *>(malloc(N * sizeof(int)));

    for(int i = 0; i < N; ++i){
        array_host[i] = i;
    }
    
    int* array_device = static_cast<int *>(malloc_device(N * sizeof(int), q));
    
    auto a = q.memcpy(array_device, array_host, N * sizeof(int));

    auto b = q.parallel_for(range<1>(N), [=](id<1> i){
        array_device[i] = 0;
    });

    auto c = q.memcpy(array_host, array_device, N * sizeof(int));

    a.precede(b);
    b.precede(c);

    q.wait();


    for(int i = 0; i < N; ++i){
        std::cout << array_host[i] << '\n'; 
    }

    free(array_device, q);
    free(array_host);

    return 0;
};
