/*

MIT License

Copyright (c) 2020-2021 Graeme Elsworthy <github@sharkshead.com>

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
#include <pthread.h>
#include <semaphore.h>
#include "config.h"

using namespace std;

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

class ExecutionEnvironment;
class Number;
class String;
class Name;
class Code;
class Pointer;

enum OperatorReturn {
  or_continue,
  or_break,
  or_return
};

class Object {
  public:
    Object();
    virtual ~Object();
    virtual Number *getNumber(LexInfo *, ExecutionEnvironment *);
    virtual String *getString(LexInfo *, ExecutionEnvironment *);
    virtual bool isName();
    virtual Name *getName(LexInfo *, ExecutionEnvironment *);
    virtual Code *getCode(LexInfo *, ExecutionEnvironment *);
    virtual Pointer *getPointer(LexInfo *, ExecutionEnvironment *);
    virtual void hold();
    virtual void release(LexInfo *);
    int referenceCount;
    virtual void debug() = 0;
};

class Number : public Object {
  public:
    Number(INT);
    Number(double);
    Number *getNumber(LexInfo *, ExecutionEnvironment *);
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
    String *getString(LexInfo *, ExecutionEnvironment *);
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
    Name *getName(LexInfo *, ExecutionEnvironment *);
    char *getValue();
    Number *getNumber(LexInfo *, ExecutionEnvironment *);
    String *getString(LexInfo *, ExecutionEnvironment *);
    Code *getCode(LexInfo *, ExecutionEnvironment *);
    Pointer *getPointer(LexInfo *, ExecutionEnvironment *);
    void debug();

  private:
    char name[MAX_NAME_LENGTH];
};

class OperationList;

class Code : public Object {
  public:
    Code(OperationList *);
    Code *getCode(LexInfo *, ExecutionEnvironment *);
    OperationList *getOperationList();
    OperatorReturn action(ExecutionEnvironment *);
    void debug();

  private:
    OperationList *operationList;
};

class ObjectList;

class Pointer : public Object {
  public:
    Pointer(Object *);
    Pointer *getPointer(LexInfo *, ExecutionEnvironment *);
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
    void setMutex(pthread_mutex_t *);
    void debug();

  private:
    CacheDebug numbers;
    CacheDebug strings;
    CacheDebug pointers;
    int unused;
    pthread_mutex_t *mutex;
};

// Start of the Operation classes

class Operation {
  public:
    Operation(LexInfo *);
    virtual OperatorReturn action(ExecutionEnvironment *) = 0;
    virtual bool isVar();
    virtual bool isFunction();
    LexInfo *getLexInfo();

  private:
    LexInfo *lexInfo;
};

class PrintStack : public Operation {
  public:
    PrintStack(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Debug : public Operation {
  public:
    Debug(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class BTreeDebug : public Operation {
  public:
    BTreeDebug(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Push : public Operation {
  public:
    Push(Object *, LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    Object *object;
};

class Pop : public Operation {
  public:
    Pop(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Swap : public Operation {
  public:
    Swap(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Dup : public Operation {
  public:
    Dup(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Int : public Operation {
  public:
    Int(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Double : public Operation {
  public:
    Double(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Plus : public Operation {
  public:
    Plus(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Minus : public Operation {
  public:
    Minus(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Times : public Operation {
  public:
    Times(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Divide : public Operation {
  public:
    Divide(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Mod : public Operation {
  public:
    Mod(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class BitwiseAnd : public Operation {
  public:
    BitwiseAnd(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class BitwiseOr : public Operation {
  public:
    BitwiseOr(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class BitwiseXor : public Operation {
  public:
    BitwiseXor(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class BitwiseNot : public Operation {
  public:
    BitwiseNot(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LeftShift : public Operation {
  public:
    LeftShift(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class RightShift : public Operation {
  public:
    RightShift(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Function : public Operation {
  public:
    Function(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
    bool isFunction();
};

class Return : public Operation {
  public:
    Return(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Var : public Operation {
  public:
    Var(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
    bool isVar();
};

class Defined : public Operation {
  public:
    Defined(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Initialised : public Operation {
  public:
    Initialised(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Assign : public Operation {
  public:
    Assign(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class PointerAssign : public Operation {
  public:
    PointerAssign(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class PointerDereference : public Operation {
  public:
    PointerDereference(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class PlusPlus : public Operation {
  public:
    PlusPlus(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class MinusMinus : public Operation {
  public:
    MinusMinus(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LessThan : public Operation {
  public:
    LessThan(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LessThanOrEquals : public Operation {
  public:
    LessThanOrEquals(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Equals : public Operation {
  public:
    Equals(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class NotEquals : public Operation {
  public:
    NotEquals(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class GreaterThanOrEquals : public Operation {
  public:
    GreaterThanOrEquals(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class GreaterThan : public Operation {
  public:
    GreaterThan(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LogicalAnd : public Operation {
  public:
    LogicalAnd(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LogicalOr : public Operation {
  public:
    LogicalOr(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class LogicalNot : public Operation {
  public:
    LogicalNot(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class True : public Operation {
  public:
    True(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class False : public Operation {
  public:
    False(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Stop : public Operation {
  public:
    Stop(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Start : public Operation {
  public:
    Start(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Break : public Operation {
  public:
    Break(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Exit : public Operation {
  public:
    Exit(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class While : public Operation {
  public:
    While(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class If : public Operation {
  public:
    If(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class IfThen : public Operation {
  public:
    IfThen(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Repeat : public Operation {
  public:
    Repeat(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    bool checkTimers(int *);
};

class Value : public Operation {
  public:
    Value(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ToName : public Operation {
  public:
    ToName(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Namespace : public Operation {
  public:
    Namespace(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Library : public Operation {
  public:
    Library(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class Print : public Operation {
  public:
    Print(bool, LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    bool newline;
};

class Printf : public Operation {
  public:
    Printf(bool, LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    bool output;
};

class Execute : public Operation {
  public:
    Execute(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
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
    OperatorReturn action(ExecutionEnvironment *);
    OperatorReturn actionLatest(ExecutionEnvironment *);
    bool isFunction();

  private:
    OperationListItem *head;
    OperationListItem *tail;
    bool newVariableStack;
    bool isFn;
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
    void setMutex(pthread_mutex_t *);

  private:
    BTreeNode *tree;
    int depth;
    int nodes;
    int entries;
    pthread_mutex_t *mutex;
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

class ExecutionEnvironment {
  public:
    VariableStack variableStack;
    Stack stack;
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
