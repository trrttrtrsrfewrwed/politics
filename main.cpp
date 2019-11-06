#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>
const size_t shift = 48;

struct AhoCorasickNode {
  // Stores if string ends at this node_.
  bool is_terminal_ = false;
  // Stores tree structure of nodes.
  std::map<char, AhoCorasickNode> trie_transitions_;

  // Stores cached transitions of the automaton_, contains
  // only pointers to the elements of trie_transitions_.
  std::unordered_map<char, AhoCorasickNode *> automaton_transitions_cache_;

  AhoCorasickNode *suffix_link_ = nullptr;
  AhoCorasickNode *terminal_link_ = nullptr;
};

AhoCorasickNode *GetAutomatonTransition(AhoCorasickNode *node, char ch) {
  AhoCorasickNode *answer;
  if (node->automaton_transitions_cache_.find(ch) !=
      node->automaton_transitions_cache_.end()) {
    answer = node->automaton_transitions_cache_[ch];
  } else {
    if (node->suffix_link_ == nullptr) {
      answer = node;
    } else {
      answer = GetAutomatonTransition(node->suffix_link_, ch);
      node->automaton_transitions_cache_[ch] = answer;
    }
  }
  return answer;
}

class NodeReference {
 public:
  NodeReference() = default;
  explicit NodeReference(AhoCorasickNode *node) : node_(node) {}

  NodeReference Next(char ch) const {
    return NodeReference(GetAutomatonTransition(this->node_, ch));
  }

  void SetTerminal(bool flg) {
    this->node_->is_terminal_ = flg;
  }

  template<class Callback>
  void ForEachMatch(Callback cb) const {
    AhoCorasickNode *curr = node_;
    while (curr != nullptr) {
      cb(curr->is_terminal_);
      curr = curr->terminal_link_;
    }
  }

 private:
  AhoCorasickNode *node_ = nullptr;
};

class AhoCorasick {
 public:
  AhoCorasick() = default;
  AhoCorasick(const AhoCorasick &) = delete;
  AhoCorasick &operator=(const AhoCorasick &) = delete;
  AhoCorasick(AhoCorasick &&) = delete;
  AhoCorasick &operator=(AhoCorasick &&) = delete;

  NodeReference Root() const { return NodeReference(&root_); }

 private:
  friend class AhoCorasickBuilder;

  mutable AhoCorasickNode root_;
};

class AhoCorasickBuilder {
 public:
  void AddString(std::string string) {
    strings_.push_back(std::move(string));
  }

  std::unique_ptr<AhoCorasick> Build() {
    auto automaton = std::make_unique<AhoCorasick>();
    automaton->root_ = AhoCorasickNode();
    for (const auto &string: strings_) {
      AddString(&automaton->root_, string);
    }
    CalculateLinks(&automaton->root_);
    return automaton;
  }

  std::vector<NodeReference> GetSurnameVertexes(){
    return surname_vertexes_;
  }

 private:
  void AddString(AhoCorasickNode *root, const std::string &string) {
    AhoCorasickNode *curr = root;
    for (auto symbol : string) {
      if (curr->trie_transitions_.find(symbol) ==
          curr->trie_transitions_.end()) {
        curr->trie_transitions_[symbol] = AhoCorasickNode();
        curr->automaton_transitions_cache_[symbol] =
            &curr->trie_transitions_[symbol];
      }
      curr = &curr->trie_transitions_[symbol];
    }
    surname_vertexes_.emplace_back(NodeReference(curr));
    curr->is_terminal_ = true;
  }

  static void CalculateLinks(AhoCorasickNode *root) {
    std::queue<AhoCorasickNode *> bfs_queue;
    root->suffix_link_ = nullptr;
    root->terminal_link_ = nullptr;
    bfs_queue.push(root);
    while (!bfs_queue.empty()) {
      auto curr = bfs_queue.front();
      bfs_queue.pop();
      for (auto &trie_transition : curr->trie_transitions_) {
        bfs_queue.push(&trie_transition.second);
        auto last_suit_suffix = curr->suffix_link_;
        while (last_suit_suffix != nullptr) {
          if (last_suit_suffix->trie_transitions_.find(trie_transition.first) !=
              last_suit_suffix->trie_transitions_.end()) {
            trie_transition.second.suffix_link_ =
                &(last_suit_suffix->trie_transitions_.find(
                    trie_transition.first))
                    ->second;
            break;
          }
          last_suit_suffix = last_suit_suffix->suffix_link_;
        }
        if (last_suit_suffix == nullptr) {
          trie_transition.second.suffix_link_ = root;
        }
        trie_transition.second.terminal_link_ =
            (!trie_transition.second.suffix_link_->is_terminal_)
            ? trie_transition.second.suffix_link_->terminal_link_
            : trie_transition.second.suffix_link_;
      }
    }
  }

  std::vector<NodeReference> surname_vertexes_;
  std::vector<std::string> strings_;
};

class PolitizationCounter {
 public:
  PolitizationCounter(size_t n, size_t k) : issues_cnt_(n), members_cnt_(k) {
    Init();
    state_ = automaton_->Root();
  }

  template<class Callback>
  void RequestProcess(Callback cb) {
    std::string issue;
    for (size_t i = 0; i < issues_cnt_; ++i) {
      std::cin >> issue;
      if (issue[0] == '+' || issue[0] == '-') {
        size_t num = 0;
        for (size_t j = 1; j < issue.length(); j++){
          num = num * 10 + size_t(issue[j] - shift);
        }
        if (issue[0] == '+') {
          Include(num);
        }
        else if (issue[0] == '-') {
          Exclude(num);
        }
      } else {
        cb(AnswerIssue(issue));
      }
    }
  }

 private:
  void Include(size_t surname_idx) {
    surname_vertexes_[surname_idx - 1].SetTerminal(true);
  }

  void Exclude(size_t surname_idx) {
    surname_vertexes_[surname_idx - 1].SetTerminal(false);
  }

  size_t AnswerIssue(const std::string& issue) {
    long long cnt = 0;
    for (size_t i = 1; i < issue.size(); ++i) {
      state_ = state_.Next(issue[i]);
      state_.ForEachMatch([&](bool is_terminal) {
        if (is_terminal) {
          ++cnt;
        }
      });
    }
    state_ = automaton_->Root();
    return cnt;
  }

  void Init() {
    auto builder = AhoCorasickBuilder();
    std::string surname;
    for (size_t i = 0; i < members_cnt_; ++i) {
      std::cin >> surname;
      builder.AddString(surname);
    }
    automaton_ = builder.Build();
    surname_vertexes_ = builder.GetSurnameVertexes();
  }

  size_t members_cnt_;
  size_t issues_cnt_;
  std::vector<NodeReference> surname_vertexes_;  // по номеру фамилии возвращает ссылку на вершину,
  // на которой заканчивается данная фамилия
  std::unique_ptr<AhoCorasick> automaton_;
  NodeReference state_;
};

int main() {
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);
  size_t n;
  size_t k;

  std::cin >> n;
  std::cin >> k;
  PolitizationCounter cnt = PolitizationCounter(n, k);
  cnt.RequestProcess([](long long cnt) { std::cout << cnt << "\n";});
}
