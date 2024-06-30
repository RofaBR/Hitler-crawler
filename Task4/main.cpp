#include <iostream>
#include <string>
#include <curl/curl.h>
#include <gumbo.h>
#include <queue>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <condition_variable>

const std::string HITLER_URL = "https://en.wikipedia.org/wiki/Adolf_Hitler";
const int MAX_HOPS = 6;

std::mutex url_queue_mutex;
std::mutex cout_mutex;
std::condition_variable cv;
std::atomic<bool> found_hitler(false);
std::unordered_map<std::string, std::string> parent_map;
std::string final_path;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

std::string fetchURL(const std::string& url) {
    CURL* curl;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

void search_for_links(GumboNode* node, std::vector<std::string>& links) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    GumboAttribute* href;
    if (node->v.element.tag == GUMBO_TAG_A &&
        (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
        std::string link(href->value);
        if (link.find("/wiki/") == 0 && link.find(":") == std::string::npos) {
            links.push_back("https://en.wikipedia.org" + link);
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_links(static_cast<GumboNode*>(children->data[i]), links);
    }
}

void build_path(const std::string& end_url) {
    std::string url = end_url;
    std::vector<std::string> path;
    while (url != "") {
        path.push_back(url);
        url = parent_map[url];
    }
    std::reverse(path.begin(), path.end());
    final_path = "";
    for (const auto& p : path) {
        final_path += p + " / ";
    }
}

void scan_queue_for_hitler(std::queue<std::pair<std::string, int>>& url_queue,
    std::unordered_set<std::string>& visited_urls) {
    while (true) {
        std::pair<std::string, int> current_entry;
        {
            std::unique_lock<std::mutex> lock(url_queue_mutex);
            cv.wait(lock, [&url_queue] { return !url_queue.empty() || found_hitler.load(); });

            if (found_hitler.load()) {
                return;
            }

            if (url_queue.empty()) {
                continue;
            }

            current_entry = url_queue.front();
            url_queue.pop();
        }

        std::string current_url = current_entry.first;
        int current_depth = current_entry.second;

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Thread " << std::this_thread::get_id() << " processing URL: " << current_url << "\n";
        }

        {
            std::lock_guard<std::mutex> lock(url_queue_mutex);
            if (visited_urls.find(current_url) != visited_urls.end()) {
                continue;
            }
            visited_urls.insert(current_url);
        }

        std::string html = fetchURL(current_url);
        if (html.empty()) {
            continue;
        }

        GumboOutput* output = gumbo_parse(html.c_str());
        if (!output) {
            continue;
        }

        std::vector<std::string> links;
        search_for_links(output->root, links);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Thread " << std::this_thread::get_id() << " found " << links.size() << " links on " << current_url << "\n";
        }

        for (const auto& link : links) {
            if (link == HITLER_URL) {
                found_hitler.store(true);
                {
                    std::lock_guard<std::mutex> lock(url_queue_mutex);
                    parent_map[link] = current_url;
                }
                build_path(link);
                cv.notify_all();
                return;
            }
            if (current_depth < MAX_HOPS - 1) {
                std::lock_guard<std::mutex> lock(url_queue_mutex);
                if (visited_urls.find(link) == visited_urls.end()) {
                    url_queue.push(std::make_pair(link, current_depth + 1));
                    parent_map[link] = current_url;
                }
            }
        }
        cv.notify_all();
    }
}

void search_in_parallel(const std::string& start_url, int num_threads) {
    std::queue<std::pair<std::string, int>> url_queue;
    std::unordered_set<std::string> visited_urls;
    url_queue.push(std::make_pair(start_url, 0));
    parent_map[start_url] = "";

    std::cout << "Starting search with " << num_threads << " threads.\n";

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&url_queue, &visited_urls]() {
            scan_queue_for_hitler(url_queue, visited_urls);
            });
    }

    for (auto& t : threads) {
        t.join();
    }

    if (!found_hitler.load()) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Hitler not found within " << MAX_HOPS << " hops.\n";
    }
    else {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Path to Hitler page: " << final_path << "\n";
    }
}

int main() {
    std::string start_url;
    int num_threads = std::thread::hardware_concurrency() / 2;

    std::cout << "Enter the Wikipedia page URL: ";
    std::cin >> start_url;

    std::cout << "Enter the number of threads (default is " << num_threads << "): ";
    std::cin >> num_threads;

    if (num_threads < 1) {
        num_threads = 1;
    }
    else if (num_threads > std::thread::hardware_concurrency()) {
        num_threads = std::thread::hardware_concurrency();
    }

    search_in_parallel(start_url, num_threads);

    return 0;
}
