#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 8;


class syclNode{
public:
    template <typename C, typename... ArgsT>
    syclNode(C&&, ArgsT&&...);

private:
    std::string _name;

    std::vector<syclNode*> _successors;

    void _precede(syclNode* v){
        _successors.push_back(v);
    }
};


class syclTask{
public:

    const std::string& name() const { 
        return _node->_name;  
    }

    syclTask& name(const std::string& name){
        _node->_name = name;
        return *this  
    }

    size_t num_successors() const {
        return _node->_successors.size();  
    }

    bool empty() const{
        return _node == nullptr;
    }

    template <typename... Ts>
    syclTask& precede(Ts&&... tasks){
        (_node->_precede(tasks._node), ...);
        return *this;
    }

    template <typename... Ts>
    syclTask& succeed(Ts&&... tasks){
        (tasks._node->_precede(_node), ...);
        return *this;
    }

private:
    syclTask* _node {nullptr};

    syclTask(syclNode*): _node {node} {}
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
