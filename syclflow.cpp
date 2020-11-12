#include <CL/sycl.hpp>
#include <vector>
#include <stack>
#include <variant>
#include <queue>
#include <map>


using namespace sycl;

static const int N = 8;


class syclFlow;
class syclNode;
class syclTask;


class syclNode {
  friend class syclTask;
  friend class syclFlow;
  friend class syclGraph;

  public:
    struct Memcpy {
      void* tgt;
      void* src;
      size_t bytes;

      Memcpy() = default;
      Memcpy(void* tgt, void* src, size_t bytes) : tgt {tgt}, src {src}, bytes {bytes} {}
    };

    struct ParallelFor {
      std::function<event(sycl::queue&)> work;

      template <typename C>
      ParallelFor(C&& work) : work {std::forward<C>(work)} {}
    };

    struct Memset {
      void* dst;
      int v;
      size_t bytes;

      Memset(void* dst, int v, size_t bytes) : dst {dst}, v {v}, bytes {bytes} {}
    };
    
    syclNode() = default;

    template <typename... ArgsT>
    syclNode(ArgsT&&... args) : _handle {std::forward<ArgsT>(args)...} {}
    
  private:

    std::variant<Memcpy, Memset, ParallelFor> _handle;

    std::string _name;
  
    std::vector<syclNode*> _successors;

    std::vector<syclNode*> _predecessors;

    bool _visited;

    sycl::event _event;

    void _precede(syclNode* v) {
      _successors.push_back(v);
      (v->_predecessors).push_back(this);
    }
};



class syclGraph {
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

    void topology_sort_helper(syclNode* n, std::stack<syclNode*>& Stack){
      n->_visited = true;
        
      for (auto s : (n->_successors))
        if (!s->_visited)  topology_sort_helper(s, Stack);

      Stack.push(n);
    }

    void topology_sort() {
      std::stack<syclNode*> Stack;

      for (auto n : _nodes)  n->_visited = false;

      for (auto n : _nodes)
        if(!n->_visited)  topology_sort_helper(n, Stack);
    
        
      while (Stack.empty() == false) {
        _nodes_sorted.push_back(Stack.top()); 
        Stack.pop(); 
      }     
    }

    void topology_sort_iterative() {
      std::map<syclNode*, int> in_degree;
      std::queue<syclNode*> q;
      
      for (auto n : _nodes) {
        for (auto s : (n->_successors)) {
          in_degree[s]++;
        }
      }

      for (auto n : _nodes) {
        if (in_degree[n] == 0) {
          q.push(n);
        }
      }

      while (!q.empty()) {
        syclNode* n = q.front();
        q.pop();
        _nodes_sorted.push_back(n);

        for (auto s : (n->_successors)) {
          if (--in_degree[s] == 0) {
            q.push(s);
          }
        }
      }
    }

private:
    std::vector<syclNode*> _nodes;

    std::vector<syclNode*> _nodes_sorted;
};



class syclTask {
  friend class syclFlow;

  public:
    const std::string& name() const { 
      return _node->_name;  
    }

    syclTask& name(const std::string& name) { 
      _node->_name = name; 

      return *this;
    }

    size_t num_successors() const { 
      return _node->_successors.size(); 
    }
        
    template <typename... Ts>
    syclTask& precede(Ts&&... tasks) {
      (_node->_precede(tasks._node), ...);    
            
      return *this;      
    };

    syclTask() = default;

  private:
    syclTask(syclNode* node): _node {node} {}

    syclNode* _node {nullptr};
};


class syclFlow {
  public:
    
    syclTask memcpy(void* tgt, void* src, size_t bytes) {
      auto node = _graph.emplace_back(
        std::in_place_type_t<syclNode::Memcpy>{}, tgt, src, bytes
      );

      return syclTask(node);
    }

    template <typename R, typename K>
    syclTask parallel_for(R&& range, K&& kernel) { 
      auto node = _graph.emplace_back(
        std::in_place_type_t<syclNode::ParallelFor>{},
        [r=std::forward<R>(range), kernel=std::forward<K>(kernel)] (sycl::queue& queue) {
          return queue.parallel_for(r, kernel);
        }
      );

      return syclTask(node);
    }

    template <typename T>
    syclTask memset(T* dst, int v, size_t bytes) {
      auto node = _graph.emplace_back(
        std::in_place_type_t<syclNode::Memset>{}, dst, v, bytes
      );

      return syclTask(node);
    }


    void run() {
        _graph.topology_sort_iterative();

        for (auto node : _graph._nodes_sorted) {
          
          for (auto pred : node->_predecessors) {
            pred->_event.wait();
          }

          std::visit(
            [node, this] (auto&&data) {
              using T = std::decay_t<decltype(data)>;
                    
              if constexpr(std::is_same_v<T, syclNode::Memcpy>) {
                auto& h = std::get<syclNode::Memcpy>(node->_handle);
                node->_event = _queue.memcpy(h.tgt, h.src, h.bytes);
              }
                    
              else if constexpr(std::is_same_v<T, syclNode::Memset>) {
                auto& h = std::get<syclNode::Memset>(node->_handle);
                node->_event = _queue.memset(h.dst, h.v, h.bytes);
              }

              else if constexpr(std::is_same_v<T, syclNode::ParallelFor>) {
                auto& h = std::get<syclNode::ParallelFor>(node->_handle);
                node->_event = h.work(_queue);
              }
            }, node->_handle);
        }
        _queue.wait();
    };

    syclFlow(queue& q) : _queue {q} {}

  private:
    queue& _queue;

    syclGraph _graph;
};




int main() {
  std::cout << "size of queue : " << sizeof(sycl::queue) << '\n';


  // declare a queue
  queue q;

  // declare syclFlow with an input parameter, queue
  syclFlow sf(q);

  // declare a pointer to an array at the host
  int* array_host = static_cast<int *>(malloc(N * sizeof(int)));

  // initialize the host array 
  for (int i = 0; i < N; ++i)  array_host[i] = i;
  
  // declare a pointer to an array at the device  
  int* array_device = static_cast<int *>(malloc_device(N * sizeof(int), q));
  
  // declare a task, named A, performing memory copy from array_host to array_device  
  auto a = sf.memcpy(array_device, array_host, N * sizeof(int)).name("A");
  
  // declare a task, named B, assigning each element of array_device to 0  
  auto b = sf.parallel_for(range<1>(N), [=](id<1> i){
    array_device[i] = 0;
  }).name("B");
  
  // declare a task, named C, assigning each element of array_device to 1  
  auto c = sf.parallel_for(range<1>(N), [=](id<1> i){
    array_device[i] = 1;
  }).name("C");
  
  // declare a task, named D, performing memory copy from array_device to array_host  
  auto d = sf.memcpy(array_host, array_device, N * sizeof(int)).name("D");
  
  // task A precedes B, which precedes C, which precedes D  
  a.precede(b);
  b.precede(c);
  c.precede(d);

  // execute
  sf.run();

  // print out values of array_host 
  for (int i = 0; i < N; ++i)  std::cout << array_host[i] << '\n'; 

  // free memory allocation
  free(array_device, q);
  free(array_host);

  return 0;
};
