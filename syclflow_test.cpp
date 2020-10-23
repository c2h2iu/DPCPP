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


    struct Memcpy{
        void* tgt;
        void* src;
        size_t bytes;
        sycl::event event;
    };

    struct ParallelFor{
        std::function<sycl::event(sycl::queue&, std::vector<sycl::event>&)> work;
        sycl::event event;
    };

    struct Memset{
        void* dst;
        int v;
        size_t bytes;
        sycl::event event;
    };

public:

    template <typename C, typename... ArgsT>
    syclNode(C&&, ArgsT&&...);

    syclNode() = default;

    template <typename... ArgsT>
    syclNode(ArgsT&&...);
    

private:
    //syclNode(){ 
    //    std::cout << "syclNode constructor\n";  
    //    _name = "node_"; 
    //}

    std::variant<Memcpy, Memset, ParallelFor> _handle;

    std::string _name;

    std::vector<syclNode*> _successors;

    void _precede(syclNode* v){
        _successors.push_back(v);
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
    
    template <typename T>
    syclTask memcpy(T* tgt, const T* src, size_t bytes){
        auto node = _graph.emplace_back(
            std::inplace_type_t<syclNode::Memcpy>{}, tgt, src, bytes
        );

        //handle.work = [tgt, src, bytes] (sycl::queue& queue) {
        //  queue.memecpy(tgt, src, bytes);
        //};

        //auto& h = std::get<syclNode::Memcpy>(node->_handle)
        //h.tgt = tgt;
        //h.src = src;
        //h.bytes = bytes;

        return syclTask(node);
    }

    syclTask parallel_for(...) {
      auto node = _graph.emplace_back(
        ...
      );
      // type erasure
      handle.work = [???] (sycl::queue& queue, std::vector<sycl::event>& deps) {
        return queue.parallel_for(???, deps, kernel);
      };
      return syclTask(node);
    }

    syclTask memset(T* dst, int v, size_t bytes){
        auto node = _graph.emplace_back(
            std::inplace_type_t<syclNode::Memset>, dst, v, bytes
        );
        return syclTask(node);
    }

    void run(){
        topology_sort(_graph.nodes);

        for(auto node : _graph.nodes) {
        
            // I need to make sure all the predecessors finish
            for(auto pred : node->_dependents) {
                auto& event = std::get<...>(pred->_handle).event;
                event.wait();
            }

            // case 1: if node is a memcpy node
            auto& h = std::get<syclNode::Memcpy>(node->_handle)
            auto event = _queue.memcpy(h.tgt, h.src. h.bytes);
            h.event = event;

            // case 2: if node is a parallel-for node
            auto event = _queue.parallel_for()
            auto& h = std::get<sycl::Node::ParallelFor>(node->_bandle);
            auto event = h.work(_queue, events);

            // case 3: if node is a memset
            auto& h = std::get<syclNode::Memset>(node->_handler);
            auto event = _queue.memset(h.dst, h.v, h.bytes);
            h.event = event;
        }
    };

    syclFlow(queue& q) : 
      _queue {q} {
    }


private:
    queue& _queue;

    syclGraph _graph;
};


int main(){

  std::cout << sizeof(sycl::queue) 

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
    
    a.precede(b, c, d);
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
