#include <iostream>
#include <algorithm>

template <typename T>
class AVLTree {
private:
    struct Node {
        T data;
        Node* left;
        Node* right;
        int height;

        explicit Node(T val) : data(val), left(nullptr), right(nullptr), height(1) {}
    };

    Node* root;

    // Explicit helper: Get height of a node (using if-else instead of ternary ?)
    int height(Node* node) const {
        if (node) {
            return node->height;
        }
        return 0;
    }

    // Explicit helper: Calculate balance factor of a node
    int getBalance(Node* node) const {
        if (node) {
            return height(node->left) - height(node->right);
        }
        return 0;
    }

    // Explicit helper: Right Rotation (Clockwise)
    Node* rightRotate(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;

        // Perform rotation
        x->right = y;
        y->left = T2;

        // Update heights
        y->height = std::max(height(y->left), height(y->right)) + 1;
        x->height = std::max(height(x->left), height(x->right)) + 1;

        return x; // New root of this subtree
    }

    // Explicit helper: Left Rotation (Counter-Clockwise)
    Node* leftRotate(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;

        // Perform rotation
        y->left = x;
        x->right = T2;

        // Update heights
        x->height = std::max(height(x->left), height(x->right)) + 1;
        y->height = std::max(height(y->left), height(y->right)) + 1;

        return y; // New root of this subtree
    }

    // Explicit helper: Recursive insertion with automatic balancing
    void insert(Node*& node, T val) {
        // 1. Standard BST insertion
        if (!node) {
            node = new Node(val);
            return;
        }
        if (val < node->data) {
            insert(node->left, val);
        } else if (val > node->data) {
            insert(node->right, val);
        } else {
            return; // Duplicate values ignored
        }

        // 2. Update current node height
        node->height = 1 + std::max(height(node->left), height(node->right));

        // 3. Get balance factor to check if node became unbalanced
        int balance = getBalance(node);

        // 4. Perform rotations if unbalanced (4 cases)

        // Left Left Case
        if (balance > 1 && val < node->left->data) {
            node = rightRotate(node);
        }
        // Right Right Case
        else if (balance < -1 && val > node->right->data) {
            node = leftRotate(node);
        }
        // Left Right Case
        else if (balance > 1 && val > node->left->data) {
            node->left = leftRotate(node->left);
            node = rightRotate(node);
        }
        // Right Left Case
        else if (balance < -1 && val < node->right->data) {
            node->right = rightRotate(node->right);
            node = leftRotate(node);
        }
    }

    // Explicit helper: Recursive node removal with rebalancing
    bool remove(Node*& node, T val) {
        if (!node) {
            return false;
        }

        bool removed = false;
        if (val < node->data) {
            removed = remove(node->left, val);
        } else if (val > node->data) {
            removed = remove(node->right, val);
        } else {
            removed = true;
            // Node found
            if (!node->left || !node->right) {
                Node* temp = nullptr;
                if (node->left) {
                    temp = node->left;
                } else {
                    temp = node->right;
                }

                if (!temp) { // No child
                    temp = node;
                    node = nullptr;
                    delete temp;
                } else { // One child
                    Node* old = node;
                    node = temp;
                    delete old;
                }
            } else {
                // Two children: Find in-order successor (min in right subtree)
                Node* current = node->right;
                while (current->left) {
                    current = current->left;
                }
                node->data = current->data;
                remove(node->right, node->data);
            }
        }

        if (!node) {
            return removed;
        }

        // Update height
        node->height = 1 + std::max(height(node->left), height(node->right));

        // Get balance factor
        int balance = getBalance(node);

        // Rebalance tree if necessary
        // Left Left Case
        if (balance > 1 && getBalance(node->left) >= 0) {
            node = rightRotate(node);
        }
        // Left Right Case
        else if (balance > 1 && getBalance(node->left) < 0) {
            node->left = leftRotate(node->left);
            node = rightRotate(node);
        }
        // Right Right Case
        else if (balance < -1 && getBalance(node->right) <= 0) {
            node = leftRotate(node);
        }
        // Right Left Case
        else if (balance < -1 && getBalance(node->right) > 0) {
            node->right = rightRotate(node->right);
            node = leftRotate(node);
        }

        return removed;
    }

    // Explicit helper for recursive search
    bool search(Node* node, T val) const {
        if (!node) {
            return false;
        }
        if (node->data == val) {
            return true;
        }
        if (val < node->data) {
            return search(node->left, val);
        }
        return search(node->right, val);
    }

    // Explicit helper to destroy tree recursively (for destructor)
    void destroyTree(Node* node) {
        if (node) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }

    // Explicit helper for in-order traversal print
    void inorder(Node* node) const {
        if (!node) {
            return;
        }
        inorder(node->left);
        std::cout << node->data << " ";
        inorder(node->right);
    }

public:
    AVLTree() : root(nullptr) {}

    ~AVLTree() {
        destroyTree(root);
    }

    // Prevent copying bugs with raw pointers
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    void insert(T val) {
        insert(root, val);
    }

    bool remove(T val) {
        return remove(root, val);
    }

    bool search(T val) const {
        return search(root, val);
    }

    void printInorder() const {
        inorder(root);
        std::cout << std::endl;
    }
};

// --- Example Usage ---
int main() {
    AVLTree<int> avl;

    for (int i = 1; i <= 9; ++i) {
        avl.insert(i);
    }

    std::cout << "In-order traversal of balanced AVL tree: ";
    avl.printInorder();

    std::cout << "\nSearching for 7: ";
    if (avl.search(7)) {
        std::cout << "Found";
    } else {
        std::cout << "Not Found";
    }
    std::cout << "\n";

    std::cout << "Removing root (4)...\n";
    avl.remove(4);

    std::cout << "In-order after removal: ";
    avl.printInorder();

    return 0;
}