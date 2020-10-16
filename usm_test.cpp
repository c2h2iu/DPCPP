#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 4;


class syclFlow{
public:
    
    queue deviceQueue{default_selector{}};
    
    template <typename R, typename C>
    void parallel_for(R& range, C&& callable);

    syclFlow(){    std::cout << "constructor\n"; }
private:
    //cl::sycl:queue deviceQueu{cl::sycl::default_selector{}};


};


int main(){
    syclFlow sf;
    queue q;
    int* d1 = malloc_shared<int>(N, sf.deviceQueue);
    int* d2 = malloc_shared<int>(N, sf.deviceQueue);

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
