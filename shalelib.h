/*

MIT License

Copyright (c) 2020 Graeme Elsworthy <github@sharkshead.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef __SHALELIB

#define __SHALELIB

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <exception>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <dlfcn.h>

using namespace std;

#define INT long int

#define MAX_NAME_LENGTH    64

#define BTREE_DATA_COUNT    6

// The LexInfo and Exception class tie an execution error back to the input.

class LexInfo {
  public:
    LexInfo(const char *, int, const char *, int);
    const char *getFilename();
    int getLineNumber();
    const char *getLine();
    int getIndex();

  private:
    const char *filename;
    int lineNumber;
    const char *line;
    int index;
};

class Exception {
  public:
    Exception();
    void chuck(const char *, LexInfo *);
    void printError();

  private:
    LexInfo *lexInfo;
    const char *message;
};

class Number;
class String;
class Name;
class Code;
class Pointer;

class Object {
  public:
    Object();
    virtual ~Object();
    virtual Number *getNumber(LexInfo *);
    virtual String *getString(LexInfo *);
    virtual bool isName();
    virtual Name *getName(LexInfo *);
    virtual Code *getCode(LexInfo *);
    virtual Pointer *getPointer(LexInfo *);
    virtual void hold();
    virtual void release(LexInfo *);
    int referenceCount;
    virtual void debug() = 0;
};

class Number : public Object {
  public:
    Number(INT);
    Number(double);
    Number *getNumber(LexInfo *);
    bool isInt();
    INT getInt();
    void setInt(INT);
    double getDouble();
    void setDouble(double);
    void release(LexInfo *);
    void debug();

  private:
    bool intRep;
    INT valueInt;
    double valueDouble;
};

class String : public Object {
  public:
    String(const char *);
    String(const char *, bool);
    ~String();
    String *getString(LexInfo *);
    const char *getValue();
    bool getRemoveStringFlag();
    void setString(const char *);
    void setRemoveStringFlag(bool);
    void release(LexInfo *);
    void debug();

  private:
    const char *str;
    bool removeStringFlag;
};

class Name : public Object {
  public:
    Name(const char *);
    bool isName();
    Name *getName(LexInfo *);
    char *getValue();
    Number *getNumber(LexInfo *);
    String *getString(LexInfo *);
    Code *getCode(LexInfo *);
    Pointer *getPointer(LexInfo *);
    void debug();

  private:
    char name[MAX_NAME_LENGTH];
};

class OperationList;

class Code : public Object {
  public:
    Code(OperationList *);
    Code *getCode(LexInfo *);
    OperationList *getOperationList();
    bool action();
    void debug();

  private:
    OperationList *operationList;
};

class ObjectList;

class Pointer : public Object {
  public:
    Pointer(Object *);
    Pointer *getPointer(LexInfo *);
    Object *getObject();
    void setObject(Object *);
    void hold();
    void release(LexInfo *);
    void debug();

  private:
    Object *object;
};

class ObjectBag {
  public:
    ObjectBag();
    Object *object;
    ObjectBag *next;
};

class CacheDebug {
  public:
    CacheDebug();
    void incUsed();
    void decUsed();
    void incUnused();
    void decUnused();
    void incNew();
    void debug();

  private:
    int used;
    int news;
};

class Cache {
  public:
    Cache();
    Number *newNumber(INT);
    Number *newNumber(double);
    void deleteNumber(Number *);
    String *newString(const char *);
    String *newString(const char *, bool);
    void deleteString(String *);
    Pointer *newPointer(Object *);
    void deletePointer(Pointer *);
    ObjectBag *usedNumbers;
    ObjectBag *usedStrings;
    ObjectBag *usedPointers;
    ObjectBag *unusedBags;
    void incUnused();
    void decUnused();
    void debug();

  private:
    CacheDebug numbers;
    CacheDebug strings;
    CacheDebug pointers;
    int unused;
};

// Start of the Operation classes

class Operation {
  public:
    Operation(LexInfo *);
    virtual bool action() = 0;
    virtual bool isVar();
    LexInfo *getLexInfo();

  private:
    LexInfo *lexInfo;
};

class PrintStack : public Operation {
  public:
    PrintStack(LexInfo *);
    bool action();
};

class Debug : public Operation {
  public:
    Debug(LexInfo *);
    bool action();
};

class BTreeDebug : public Operation {
  public:
    BTreeDebug(LexInfo *);
    bool action();
};

class Push : public Operation {
  public:
    Push(Object *, LexInfo *);
    bool action();

  private:
    Object *object;
};

class Pop : public Operation {
  public:
    Pop(LexInfo *);
    bool action();
};

class Swap : public Operation {
  public:
    Swap(LexInfo *);
    bool action();
};

class Dup : public Operation {
  public:
    Dup(LexInfo *);
    bool action();
};

class Int : public Operation {
  public:
    Int(LexInfo *);
    bool action();
};

class Double : public Operation {
  public:
    Double(LexInfo *);
    bool action();
};

class Plus : public Operation {
  public:
    Plus(LexInfo *);
    bool action();
};

class Minus : public Operation {
  public:
    Minus(LexInfo *);
    bool action();
};

class Times : public Operation {
  public:
    Times(LexInfo *);
    bool action();
};

class Divide : public Operation {
  public:
    Divide(LexInfo *);
    bool action();
};

class Mod : public Operation {
  public:
    Mod(LexInfo *);
    bool action();
};

class BitwiseAnd : public Operation {
  public:
    BitwiseAnd(LexInfo *);
    bool action();
};

class BitwiseOr : public Operation {
  public:
    BitwiseOr(LexInfo *);
    bool action();
};

class BitwiseXor : public Operation {
  public:
    BitwiseXor(LexInfo *);
    bool action();
};

class BitwiseNot : public Operation {
  public:
    BitwiseNot(LexInfo *);
    bool action();
};

class Var : public Operation {
  public:
    Var(LexInfo *);
    bool action();
    bool isVar();
};

class Defined : public Operation {
  public:
    Defined(LexInfo *);
    bool action();
};

class Initialised : public Operation {
  public:
    Initialised(LexInfo *);
    bool action();
};

class Assign : public Operation {
  public:
    Assign(LexInfo *);
    bool action();
};

class PointerAssign : public Operation {
  public:
    PointerAssign(LexInfo *);
    bool action();
};

class PointerDereference : public Operation {
  public:
    PointerDereference(LexInfo *);
    bool action();
};

class LessThan : public Operation {
  public:
    LessThan(LexInfo *);
    bool action();
};

class LessThanOrEquals : public Operation {
  public:
    LessThanOrEquals(LexInfo *);
    bool action();
};

class Equals : public Operation {
  public:
    Equals(LexInfo *);
    bool action();
};

class NotEquals : public Operation {
  public:
    NotEquals(LexInfo *);
    bool action();
};

class GreaterThanOrEquals : public Operation {
  public:
    GreaterThanOrEquals(LexInfo *);
    bool action();
};

class GreaterThan : public Operation {
  public:
    GreaterThan(LexInfo *);
    bool action();
};

class LogicalAnd : public Operation {
  public:
    LogicalAnd(LexInfo *);
    bool action();
};

class LogicalOr : public Operation {
  public:
    LogicalOr(LexInfo *);
    bool action();
};

class LogicalNot : public Operation {
  public:
    LogicalNot(LexInfo *);
    bool action();
};

class True : public Operation {
  public:
    True(LexInfo *);
    bool action();
};

class False : public Operation {
  public:
    False(LexInfo *);
    bool action();
};

class Stop : public Operation {
  public:
    Stop(LexInfo *);
    bool action();
};

class Start : public Operation {
  public:
    Start(LexInfo *);
    bool action();
};

class Break : public Operation {
  public:
    Break(LexInfo *);
    bool action();
};

class Exit : public Operation {
  public:
    Exit(LexInfo *);
    bool action();
};

class While : public Operation {
  public:
    While(LexInfo *);
    bool action();
};

class If : public Operation {
  public:
    If(LexInfo *);
    bool action();
};

class IfThen : public Operation {
  public:
    IfThen(LexInfo *);
    bool action();
};

class Repeat : public Operation {
  public:
    Repeat(LexInfo *);
    bool action();

  private:
    bool checkTimers(int *);
};

class Value : public Operation {
  public:
    Value(LexInfo *);
    bool action();
};

class ToName : public Operation {
  public:
    ToName(LexInfo *);
    bool action();
};

class Namespace : public Operation {
  public:
    Namespace(LexInfo *);
    bool action();
};

class Library : public Operation {
  public:
    Library(LexInfo *);
    bool action();
};

class Print : public Operation {
  public:
    Print(bool, LexInfo *);
    bool action();

  private:
    bool newline;
};

class Printf : public Operation {
  public:
    Printf(bool, LexInfo *);
    bool action();

  private:
    bool output;
};

class Execute : public Operation {
  public:
    Execute(LexInfo *);
    bool action();
};

class OperationListItem {
  public:
    OperationListItem(Operation *);
    OperationListItem *getNext();
    void setNext(OperationListItem *);
    Operation *getOperation();

  private:
    Operation *operation;
    OperationListItem *next;
};

class OperationList {
  public:
    OperationList();
    void addOperation(Operation *);
    OperationListItem *getOperationList();
    bool action();
    bool actionLatest();

  private:
    OperationListItem *head;
    OperationListItem *tail;
    bool newVariableStack;
};

class ObjectListItem {
  public:
    ObjectListItem(Object *);
    ObjectListItem *getNext();
    void setNext(ObjectListItem *);
    Object *getObject();

  private:
    Object *object;
    ObjectListItem *next;
};

class ObjectList {
  public:
    ObjectList();
    int getLength();
    void addObject(Object *);
    Object *getObject(int);

  private:
    int length;
    ObjectListItem *head;
    ObjectListItem *tail;
};

class Variable {
  public:
    Variable(const char *);
    ~Variable();
    char *getName();
    Object *getObject();
    void setObject(Object *);
    bool isInitialised();
    Variable *getNext();
    void setNext(Variable *);

  private:
    char *name;
    Object *object;
    Variable *next;
};

class VariableStackItem {
  public:
    VariableStackItem(VariableStackItem *);
    ~VariableStackItem();
    VariableStackItem *getDown();
    void setDown(VariableStackItem *);
    Variable *addVariable(char *);
    Variable *addVariable(char *, LexInfo *);
    Variable *findVariable(char *);

  private:
    Variable *list;
    VariableStackItem *down;
};

class VariableStack {
  public:
    VariableStack();
    void addVariableStack();
    void popVariableStack();
    Variable *addVariable(char *);
    Variable *addVariable(char *, LexInfo *);
    Variable *findVariable(char *);
    bool isEmpty();

  private:
    VariableStackItem *head;
};

class BTreeNode {
  public:
    BTreeNode(bool);
    BTreeNode(bool, BTreeNode *, Variable *, BTreeNode *);
    bool isLeaf();
    int getNumber();
    Variable *getData(int);
    void setData(int, Variable *);
    BTreeNode *getPointer(int);
    void incNumber();
    void setPointer(int, BTreeNode *);
    bool addData(Variable *, BTreeNode **, Variable **, BTreeNode **, int *);
    bool addDataToNode(BTreeNode *, Variable *, BTreeNode *, BTreeNode **, Variable **, BTreeNode **, int *);

  private:
    const bool leaf;
    int number;
    Variable *data[BTREE_DATA_COUNT];
    BTreeNode *pointer[BTREE_DATA_COUNT + 1];
};

class BTree {
  public:
    BTree();
    bool addVariable(Variable *);
    Variable *findVariable(const char *);
    void debug();
    void print();

  private:
    BTreeNode *tree;
    int depth;
    int nodes;
    int entries;
    void printTree(BTreeNode *);
    void printDetail(BTreeNode *, int);
};

class StackItem {
  public:
    StackItem();
    Object *getObject();
    void setObject(Object *);
    StackItem *getDown();
    void setDown(StackItem *);

  private:
    Object *object;
    StackItem *down;
};

class Stack {
  public:
    Stack();
    void push(Object *);
    Object *pop(LexInfo *);
    StackItem *getStack();
    int getStackSize();
    int getUsedSize();
    void debug();

  public:
    StackItem *stack;
    StackItem *used;
    int stackSize;
    int usedSize;
};

// Plugin support classes

class Plugin {
  public:
    virtual void init() = 0;
};

class PluginSupport {
  public:
    PluginSupport(BTree *);
    BTree *getBTree();

  private:
    BTree *btree;
};

extern Cache cache;
extern Stack stack;
extern BTree btree;
extern Exception slexception;
extern VariableStack variableStack;

#endif /* __SHALELIB */
