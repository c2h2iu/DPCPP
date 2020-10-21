#include <CL/sycl.hpp>
using namespace sycl;

static const int N = 8;


class syclGraph{
friend class syclFlow;
friend class syclNode;
friend class syclTask;

public:
    syclGraph() = default;

    template <typename... ArgsT>
    syclNode* emplace_back(ArgsT&&... args){
        auto node = new syclNode(std::forward<ArgsT>(args)...);
        _nodes.push_back(node);
        return node;
    }

private:
    std::vector<syclNode*> _nodes;
};



class syclNode{
friend class syclTask;
friend class syclFlow;
friend class syclGraph;

public:
    template <typename C, typename... ArgsT>
    syclNode(C&&, ArgsT&&...);

    syclNode() = default;
    

private:
    //syclNode(){ 
    //    std::cout << "syclNode constructor\n";  
    //    _name = "node_"; 
    //}

    std::string _name;

    std::vector<syclNode*> _successors;

    void _precede(syclNode* v){
        _successors.push_back(v);

        for(auto s : _successors){ std::cout << s->_name << ' '; }
        std::cout << '\n';
    }
};


class syclTask{
friend class syclFlow;
public:
    const std::string& name() const { 
        return _node->_name;  
    }

    syclTask& name(const std::string& name){ 
        _node->_name = name; 
        return *this;
    }

    size_t num_successors() const { 
        return _node->_successors.size(); 
    }
        
    //syclTask(std::string name){ _name=name; }
   
    template <typename... Ts>
    syclTask& precede(Ts&&... tasks){
        (_node->_precede(tasks._node), ...);        
        return *this;      
    };

    syclTask() = default;

private:
    syclTask(syclNode* node): _node {node} {}

    syclNode* _node {nullptr};
};


class syclFlow{
public:
    
    template <typename R, typename C>
    syclTask parallel_for(R&& range, C&& callable){
        syclNode* node = new syclNode();
        syclTask task(node);

        deviceQueue.parallel_for(std::forward<R>(range), std::forward<C>(callable));  
        return task;
    }

    template <typename T>
    syclTask memcpy(T* tgt, const T* src, size_t bytes){
        syclNode* node = new syclNode();
        syclTask task(node);

        deviceQueue.memcpy(tgt, src, bytes);
        return task;
    }

    void run(){
      
    };

    syclFlow(queue& q){
        deviceQueue = q;    
    }


private:
    queue deviceQueue;
};


int main(){
    queue q;
    syclFlow sf(q);

    int* array_host = static_cast<int *>(malloc(N * sizeof(int)));

    for(int i = 0; i < N; ++i){
        array_host[i] = i;
    }
    
    int* array_device = static_cast<int *>(malloc_device(N * sizeof(int), q));
    
    auto a = sf.memcpy(array_device, array_host, N * sizeof(int)).name("A");
    
    auto b = sf.parallel_for(range<1>(N), [=](id<1> i){
        array_device[i] = 0;
    }).name("B");
    
    auto c = sf.parallel_for(range<1>(N), [=](id<1> i){
        array_device[i] = 1;
    }).name("C");
    
    auto d = sf.memcpy(array_host, array_device, N * sizeof(int)).name("D");
    
    a = a.precede(b);
    b.precede(c);
    c.precede(d);

    q.wait();

    //sf.run();

    for(int i = 0; i < N; ++i){
        std::cout << array_host[i] << '\n'; 
    }

    free(array_device, q);
    free(array_host);

    return 0;
};
