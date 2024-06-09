#include <fstream>
#include <functional>
#include <map>
#include <iostream>
#include <list>
#include <ostream>
#include <string>
#include <thread>
#include <unordered_map>
#include "json.hpp"

namespace httplib {

class Request;
class Response;
class DataSink;

using Headers = std::map<std::string, std::string>;
using Handler =
    std::function<void(const httplib::Request &, httplib::Response &)>;

class TaskQueue {
public:
  TaskQueue() = default;
  virtual ~TaskQueue() = default;

  virtual bool enqueue(std::function<void()> fn) = 0;
  virtual void shutdown() = 0;

  virtual void on_idle() {}
};

class ThreadPool final : public TaskQueue {
public:
  explicit ThreadPool(size_t n, size_t mqr = 0)
      : shutdown_(false), max_queued_requests_(mqr) {
    while (n) {
      threads_.emplace_back(worker(*this));
      n--;
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool() override = default;

  bool enqueue(std::function<void()> fn) override {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (max_queued_requests_ > 0 && jobs_.size() >= max_queued_requests_) {
        return false;
      }
      jobs_.push_back(std::move(fn));
    }

    cond_.notify_one();
    return true;
  }

  void shutdown() override {
    // Stop all worker threads...
    {
      std::unique_lock<std::mutex> lock(mutex_);
      shutdown_ = true;
    }

    cond_.notify_all();

    // Join...
    for (auto &t : threads_) {
      t.join();
    }
  }

private:
  struct worker {
    explicit worker(ThreadPool &pool) : pool_(pool) {}

    void operator()() {
      for (;;) {
        std::function<void()> fn;
        {
          std::unique_lock<std::mutex> lock(pool_.mutex_);

          pool_.cond_.wait(
              lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });

          if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }

          fn = std::move(pool_.jobs_.front());
          pool_.jobs_.pop_front();
        }

        // assert(true == static_cast<bool>(fn));
        fn();
      }
    }

    ThreadPool &pool_;
  };
  friend struct worker;

  std::vector<std::thread> threads_;
  std::list<std::function<void()>> jobs_;

  bool shutdown_;
  size_t max_queued_requests_ = 0;

  std::condition_variable cond_;
  std::mutex mutex_;
};

class DataSink {
public:
  DataSink() : os(&sb_), sb_(*this) {}

  DataSink(const DataSink &) = delete;
  DataSink &operator=(const DataSink &) = delete;
  DataSink(DataSink &&) = delete;
  DataSink &operator=(DataSink &&) = delete;

  std::function<bool(const char *data, size_t data_len)> write;
  std::function<void()> done;
  std::function<void(const Headers &trailer)> done_with_trailer;
  std::ostream os;

private:
  class data_sink_streambuf : public std::streambuf {
  public:
    explicit data_sink_streambuf(DataSink &sink) : sink_(sink) {}

  protected:
    std::streamsize xsputn(const char *s, std::streamsize n) {
      sink_.write(s, static_cast<size_t>(n));
      return n;
    }

  private:
    DataSink &sink_;
  };

  data_sink_streambuf sb_;
};

class Request {
public:
  std::string get_param_value(const std::string &key) const {
    return params.count(key) ? params.at(key) : "";
  }

  bool has_header(const std::string &key) const {
    return headers.count(key) > 0;
  }

  std::string get_header_value(const std::string &key) const {
    return headers.count(key) ? headers.at(key) : "";
  }

  bool has_param(const std::string &key) const { return params.count(key) > 0; }

  std::string body;
  std::string remote_addr;
  std::string remote_port;
  std::string status;
  std::string method;
  std::string path;
  std::map<std::string, std::string> params;
  std::unordered_map<std::string, std::string> path_params;
  std::map<std::string, std::string> headers;
};

class Response {
public:
  // Existing set_content method
  void set_content(const std::string &content,
                   const std::string &content_type) {
    this->content = content;
    this->content_type = content_type;
  }

  // New set_content method
  void set_content(const char *content, size_t content_length,
                   const std::string &content_type) {
    this->content = std::string(content, content_length);
    this->content_type = content_type;
  }

  void set_header(const std::string &key, const std::string &value) {
    headers[key] = value;
  }

  void set_chunked_content_provider(
      std::string mimeType, std::function<bool(size_t, DataSink &)> provider, std::function<void(bool)> on_complete) {
    this->chunked_content_provider = provider;
  }

  int status;
  std::string body;
  std::string content;
  std::string content_type;
  std::map<std::string, std::string> headers;
  std::function<bool(size_t, DataSink &)> chunked_content_provider;
};

// /**
//  * Captures parameters in request path and stores them in Request::path_params
//  *
//  * Capture name is a substring of a pattern from : to /.
//  * The rest of the pattern is matched agains the request path directly
//  * Parameters are captured starting from the next character after
//  * the end of the last matched static pattern fragment until the next /.
//  *
//  * Example pattern:
//  * "/path/fragments/:capture/more/fragments/:second_capture"
//  * Static fragments:
//  * "/path/fragments/", "more/fragments/"
//  *
//  * Given the following request path:
//  * "/path/fragments/:1/more/fragments/:2"
//  * the resulting capture will be
//  * {{"capture", "1"}, {"second_capture", "2"}}
//  */
// class PathParamsMatcher final : public MatcherBase {
// public:
//   PathParamsMatcher(const std::string &pattern);

//   bool match(Request &request) const override;

// private:
//   static constexpr char marker = ':';
//   // Treat segment separators as the end of path parameter capture
//   // Does not need to handle query parameters as they are parsed before path
//   // matching
//   static constexpr char separator = '/';

//   // Contains static path fragments to match against, excluding the '/' after
//   // path params
//   // Fragments are separated by path params
//   std::vector<std::string> static_fragments_;
//   // Stores the names of the path parameters to be used as keys in the
//   // Request::path_params map
//   std::vector<std::string> param_names_;
// };

void _LOG(const std::string& message) {
    std::cout << message << std::endl;
}

class Server {
public:
  // New methods
  std::function<void(const Request &, const Response &)> logger;
  std::function<void(const Request &, Response &, std::exception_ptr)>
      exception_handler;
  std::function<void(const Request &, Response &)> error_handler;
  double read_timeout;
  double write_timeout;
  std::string base_dir;
  std::function<bool(size_t, DataSink &)> chunked_content_provider;
  std::function<void()> on_complete;
  std::function<void(const Request &, Response &)> options_handler;
  std::string host;
  int port;

  std::function<TaskQueue *(void)> new_task_queue;

  void stop();

  void
  set_chunked_content_provider(const std::string &mime_type,
                               std::function<bool(size_t, DataSink &)> provider,
                               std::function<void()> on_complete) {
    this->chunked_content_provider = provider;
    this->on_complete = on_complete;
  }

  void Options(const std::string &pattern, Handler handler) {
    this->options_handler = handler;
  }

  void
  set_logger(std::function<void(const Request &, const Response &)> logger) {
    this->logger = logger;
  }

  void set_exception_handler(
      std::function<void(const Request &, Response &, std::exception_ptr)>
          handler) {
    this->exception_handler = handler;
  }

  void
  set_error_handler(std::function<void(const Request &, Response &)> handler) {
    this->error_handler = handler;
  }

  void set_read_timeout(double sec) { this->read_timeout = sec; }

  void set_write_timeout(double sec) { this->write_timeout = sec; }

  bool bind_to_port(std::string host, int port) {
    this->host = host;
    this->port = port;
    return true; // Placeholder
  }

  void set_base_dir(std::string path) { this->base_dir = path; }

  bool listen_after_bind() {
    return true; // Placeholder
  }

  Server &Post(const std::string &pattern, Handler handler) {
    post_handlers_[pattern] = handler;
    _LOG(std::string("install handler: POST ") + pattern);
    return *this;
  }

  Server &Get(const std::string &pattern, Handler handler) {
    get_handlers_[pattern] = handler;
    _LOG(std::string("install handler: GET ") + pattern);
    return *this;
  }

  void set_default_headers(const std::map<std::string, std::string> &headers) {
    default_headers_ = headers;
  }

  void printHandlers() const {
      std::cout << "POST Handlers:" << std::endl;
      for (const auto& pair : post_handlers_) {
          std::cout << "  " << pair.first << std::endl;
      }

      std::cout << "GET Handlers:" << std::endl;
      for (const auto& pair : get_handlers_) {
          std::cout << "  " << pair.first << std::endl;
      }
  }

  std::string rpc_call(const std::string &method, const std::string &path,
                       const std::string &body) {
    Request req;
    Response res;

    req.body = body;
    req.method = method;
    req.path = path;

    // printHandlers();

    std::string log_str = "RPC CALL: method=" + method + ", path=" + path + ", body='" + body + "'";
    std::cout << log_str << std::endl;

    if (method == "POST" && post_handlers_.count(path)) {
      post_handlers_[path](req, res);
    } else if (method == "GET" && get_handlers_.count(path)) {
      get_handlers_[path](req, res);
    } else {
      nlohmann::json error_response;
      error_response["status"] = "error";
      error_response["error"] = "path not found: " + path;
      return error_response.dump();
    }

     std::cout << "RESPONSE: \"" << res.content << "\"" << std::endl;

    nlohmann::json* response = new nlohmann::json({});
    (*response)["status"] = "success";
    (*response)["content"] = std::string(res.content);
    
    return response->dump();
  }

private:
  std::unordered_map<std::string, Handler> post_handlers_;
  std::unordered_map<std::string, Handler> get_handlers_;
  std::map<std::string, std::string> default_headers_;
};
} // namespace httplib