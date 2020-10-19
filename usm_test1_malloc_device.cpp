#include <CL/sycl.hpp>
#include <cmath>
using namespace sycl;

static const int N = 10;


class syclFlow{
public:
    
    queue deviceQueue{gpu_selector{}};
    std::vector<platform> platforms = platform::get_platforms();
    
    template <typename R, typename C>
    void parallel_for(R& range, C&& callable);

    syclFlow(){    std::cout << "constructor\n"; 
        //std::cout << deviceQueue.get_device().get_info<info::device::name>() << '\n';
        //for(const auto &plat:platforms){
        //    std::cout << plat.get_info<info::platform::name>() << " by vendor : "  << plat.get_info<info::platform::vendor>() << '\n';        
    }
private:
    //cl::sycl:queue deviceQueu{cl::sycl::default_selector{}};


};


int main(){
    syclFlow sf;

    queue q;

    //int* d1 = malloc_shared<int>(N, sf.deviceQueue);
    //int* d2 = malloc_shared<int>(N, sf.deviceQueue);
    int* array_host = static_cast<int *>(malloc(N*sizeof(int)));
    
    for(int i = 0; i < N; ++i){
        array_host[i] = i;
    }
    
    int* array = static_cast<int *>(malloc_device(N*sizeof(int), q));

    q.memcpy(array, array_host, sizeof(int)*N);

    auto s = q.parallel_for(range<1>(1), [=](id<1> i){
        array[0] = array[0] + 1;
    });

    auto d = q.parallel_for(range<1>(1), {s}, [=](id<1> i){
        array[1] = array[0]  + 1;
    });

    auto a = q.parallel_for(range<1>(1), {s}, [=](id<1> i){
        array[2] = array[0] + 2;
    });
    
    auto e = q.parallel_for(range<1>(1), {s}, [=](id<1> i){
        array[3] = array[0] + 3;
    });

    auto b = q.parallel_for(range<1>(1), {d, a}, [=](id<1> i){
        array[4] = array[1] + array[2];
    });

    auto c = q.parallel_for(range<1>(1), {b, e}, [=](id<1> i){
        array[5] = array[4] + array[3];
    });
    
    auto x = q.parallel_for(range<1>(1), {c, b}, [=](id<1> i){
        array[6] = array[4] + array[5];
    });

    auto y = q.parallel_for(range<1>(1), {x}, [=](id<1> i){
        array[7] = array[6] + 7;
    });

    auto z = q.parallel_for(range<1>(1), {y}, [=](id<1> i){
        array[8] = array[7] + 8;
    });
    auto t = q.parallel_for(range<1>(1), {d,z,e}, [=](id<1> i){
        array[9] = array[1] + array[8] + array[3];
    });
    
    q.memcpy(array_host, array, sizeof(int)*N);

    q.wait();

    //q.parallel_for(range<1>(N), {e0, e1, e2, e3}, [=](id<1> i){
    //}).wait();

    for(int i = 0; i < N; ++i){
        std::cout << array_host[i] << '\n'; 
    }

    //free(d1, q);
    //free(d2, q);
    free(array_host);
    free(array, q);
    return 0;
};


/***
int main(){
    queue q;

    int* d1 = malloc_shared<int>(N, q);
    int* d2 = malloc_shared<int>(N, q);

    for(int i = 0; i < N; ++i){
        d1[i] = 10;
        d2[i] = 10;
    }

    auto e1 = q.parallel_for(range<1>(N), [=](id<1> i){
        d1[i] += 2;
    });

    auto e2 = q.parallel_for(range<1>(N), [=](id<1> i){
        d2[i] += 3;
    });

    q.parallel_for(range<1>(N), {e1, e2}, [=](id<1> i){
        d1[i] += d2[i];
    }).wait();

    for(int i = 0; i < N; ++i){
        std::cout << d1[i] << '\n'; 
    }

    free(d1, q);
    free(d2, q);

    return 0;
};
***/
