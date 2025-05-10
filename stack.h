#include <cstring>
#include <iostream>

struct Node {
  Node* link = nullptr;
  char* str;
};

void Push(Node*& nextel, int& size, char* str) {
  if (size == 0) {
    nextel = new Node;
  } else {
    Node* newelem = new Node;
    newelem->link = nextel;
    nextel = newelem;
  }

  size++;
  nextel->str = str;
  std::cout << "ok\n";
}

void Pop(Node*& nextel, int& size) {
  if (size == 0) {
    std::cout << "error\n";
  } else {
    size--;
    std::cout << nextel->str;
    Node* po = nextel->link;
    delete nextel;
    nextel = po;
  }
}

void Back(Node*& nextel, int& size) {
  if (size == 0) {
    std::cout << "error\n";
  } else {
    std::cout << nextel->str;
  }
}

void Size(int& size) { std::cout << size << '\n'; }

void Clear(Node*& nextel, int& size, bool fl) {
  while (nextel != nullptr) {
    Node* po = nextel->link;
    free(nextel->str);
    delete nextel;
    nextel = po;
  }

  size = 0;
  fl == 1 ? std::cout << "ok\n" : std::cout << "bye";
}

char* GetElem () {
  char cur_elem = 0;
  int size1 = 0;
  int length = 8;
  char *elem = (char *) malloc(sizeof(char) * length);

  while (cur_elem != '\n') {
    cur_elem = fgetc(stdin);
    elem[size1++] = cur_elem;
    if (size1 == length - 2) {
      length *= 2;
      elem = (char *) realloc(elem, sizeof(char) * length);
    }
  }

  if (size1 < length) {
    elem = (char *) realloc(elem, sizeof(char) * (size1 + 1));
  }

  elem[size1] = '\0';
  std::memmove(elem, elem + 1, sizeof(char) * size1);
  return elem;
}

int main() {
  int size = 0;
  Node *nextel = nullptr;
  char command[10];
  while (true) {
    if (scanf("%s", command)) {
      if (strcmp(command, "push") == 0) {
        char* elem = GetElem();
        Push(nextel, size, elem);
      } else if (strcmp(command, "pop") == 0) {
        Pop(nextel, size);
      } else if (strcmp(command, "back") == 0) {
        Back(nextel, size);
      } else if (strcmp(command, "size") == 0) {
        Size(size);
      } else if (strcmp(command, "clear") == 0) {
        Clear(nextel, size, 1);
      } else if (strcmp(command, "exit") == 0) {
        Clear(nextel, size, 0);
        delete nextel;
        break;
      }
    }
  }
}
