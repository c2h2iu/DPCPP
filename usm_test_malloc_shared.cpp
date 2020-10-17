#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 8;


class syclFlow{
public:
    
    template <typename R, typename C>
    syclTask parallel_for(R& range, C&& callable);

    template <typename T>
    syclTask memcpy(T* tgt, const T* src, size_t num);


    syclFlow(queue& q){
        deviceQueue = q;    
        std::cout << "constructor\n"; 
    }

private:
    queue deviceQueue;
    //cl::sycl:queue deviceQueu{cl::sycl::default_selector{}};
};


int main(){
    syclFlow sf;

    queue q;

    //int* d1 = malloc_shared<int>(N, sf.deviceQueue);
    //int* d2 = malloc_shared<int>(N, sf.deviceQueue);
    //int* array = malloc_shared<int>(N, q);
    int* array = malloc_shared<int>(N,q); 

    for(int i = 0; i < N; ++i){
        array[i] = i;
    }
    
    auto b1 = q.parallel_for(range<1>(1), [=](id<1> i){
        array[0] = array[0] + 1;
    });

    auto b2 = q.parallel_for(range<1>(1), [=](id<1> i){
        array[1] = array[0]  + 1;
    });

    auto b3 = q.parallel_for(range<1>(1), {b1, b2}, [=](id<1> i){
        array[2] = array[0] + array[1];
    });
    
    auto a = q.parallel_for(range<1>(1), [=](id<1> i){
        array[4] = 200;
    });

    auto b = q.parallel_for(range<1>(1), {a, b3}, [=](id<1> i){
        array[3] = array[2] + array[4];
    });


    auto c = q.parallel_for(range<1>(1), {a}, [=](id<1> i){
        array[5] = array[4] + 5;
    });
    
    auto d = q.parallel_for(range<1>(1), {c, b}, [=](id<1> i){
        array[6] = array[3] + array[5];
    });

    auto e7 = q.parallel_for(range<1>(1), [=](id<1> i){
        array[7] = array[7] + 66;
    });

    q.wait();

    //q.parallel_for(range<1>(N), {e0, e1, e2, e3}, [=](id<1> i){
    //}).wait();

    for(int i = 0; i < N; ++i){
        std::cout << array[i] << '\n'; 
    }

    //free(d1, q);
    //free(d2, q);
    free(array);

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
