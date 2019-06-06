//
// Created by 郑文鑫 on 2019-03-09.
//

#include "utility.hpp"
#include <functional>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <cstring>
#include "exception.hpp"
#define offset_type long
const int blocksize=4096;
namespace sjtu {
template <class Key, class Value, class Compare = std::less<Key> >
class BTree {
 public:
    class iterator;
    class const_iterator;

 private:
 const static int M=blocksize/(sizeof(long)+sizeof(Key));
 const static int L=blocksize/sizeof(Value);
 const static int off=0;
 struct save{
  size_t num_of_data;
  offset_type root;//树根
  offset_type first_leaf;//第一个叶子
  offset_type last_leaf;//最后一个叶子
  offset_type eend;//文件的末尾
  save()
  {
   num_of_data=0;
   root=0;
   first_leaf=0;
   last_leaf=0;
   eend=0;
  }
  }saving;//树的信息

 struct index_t{
   Key key;
   offset_type offset;
   index_t(){offset=0;}
 };

 struct internal_node{
   offset_type offset;
   offset_type father;
   int now_size;
   bool type;//type为1时表示它的儿子是叶子结点
   index_t data[M+1];
   internal_node()
   {
     offset=father=0;
     now_size=0;
     type=false;
   }
 };//内部节点

 struct record_t
 {
   Key key;
   Value value;
 };

 struct leaf_node{
   offset_type offset;
   offset_type father;
   offset_type next;
   offset_type prev;
   int now_size;
   record_t data[L+1];
   leaf_node()
   {
     offset=next=prev=father=0;
     now_size=0;
   }
 };//叶子结点
 std::fstream ffile;
 //int open_cnt;
 bool is_open;
 bool exists;

   inline void open_file()
   {
     exists=1;
     if(is_open==0)
     {
       ffile.open("bpt",std::ios::binary);
       if(!ffile) throw invalid_iterator();
       else
           read_index(off,sizeof(save));
       is_open=1;
     }
   }

   inline char * read_index(offset_type offset,size_t siz)
   {
       ffile.seekg(offset,std::ios::beg);
       char *buffer=new char [blocksize];
       ffile.read(reinterpret_cast<int>(buffer),siz);
       return buffer;
   }
    inline char * read_leaf(offset_type offset,size_t siz)
    {
        ffile.seekg(offset,std::ios::beg);
        char *buffer=new char [blocksize];
        ffile.read(reinterpret_cast<int>(buffer),siz);
        return buffer;
    }

   inline void close_file()
   {
       if(is_open==1)
       {
           ffile.close("bpt");
           is_open=0;
       }
   }

   inline void write_index(offset_type offset,size_t siz,internal_node *node) const
   {
       ffile.seekp(offset,std::ios::beg);
       ffile.write(reinterpret_cast<char*>(node),siz);
   }

   inline void write_leaf(offset_type offset,size_t siz,leaf_node leaf) const
   {
       ffile.seekp(offset,std::ios::beg);
       ffile.write(reinterpret_cast<int>(*leaf),siz);
   }

   inline void build_bpt()
   {
       saving.num_of_data=0;
       saving.eend=sizeof(save);
       internal_node root;
       leaf_node leaf;
       saving.eend=saving.root=root.offset;
       saving.eend+=sizeof(internal_node);
       saving.first_leaf=saving.last_leaf=leaf.offset=saving.eend;
       saving.eend+=sizeof(leaf_node);
       root.father=0;
       root.now_size=1;
       root.type=1;
       root.data[0].offset=leaf.offset;
       leaf.next=leaf.prev=0;
       leaf.father=root.offset;
       leaf.now_size=0;
       write_index(off,sizeof(save),saving);
       write_index(root.offset,sizeof(internal_node),root);
       write_leaf(leaf.offset,sizeof(leaf_node),leaf);
   }

   offset_type find_location(const Key &key,offset_type offset)
   {
       internal_node node;
       node = read_index(offset, sizeof(internal_node));
       if (node.type == 1)//孩子是叶子结点
       {
           int pos;
           for (pos = 0; pos < node.now_size; ++pos)
           {if (key < node.data[pos].key) break;}

           if (pos == 0) return 0;
           else return node.data[pos - 1].offset;
       } else {
           int pos;
           for (pos = 0; pos < node.now_size; ++pos)
               if (key < node.data[pos]) break;
           if (pos == 0) return 0;
           return find_location(key, node.data[pos - 1].offset);//递归查找
       }
   }

  // Your private members go here
 public:
  typedef pair<const Key, Value> value_type;

  class iterator {
      friend class BTree;
   private:
    // Your private members go here
    offset_type offset;//偏移量
    int position;//在节点中的位置
    BTree *what_tree;
   public:
    bool modify(const Key& key){
      return true;
      ////这函数是啥
    }
    iterator() {
        offset=position=0;
        what_tree= nullptr;
    }

    iterator(BTree *x,offset_type offs=0,int p=0)
    {
        what_tree=x;
        offset=offs;
        position=p;
    }
    iterator(const iterator& other) {
      what_tree=other.what_tree;
      position=other.position;
      offset=other.offset;
    }
    // Return a new iterator which points to the n-next elements
    iterator operator++(int) {
      iterator tmp=*this;
      if(*this==what_tree->end())//最后一个数据了，++没有意义了吧
      {
          what_tree= nullptr;
          offset=0;
          position=0;
          return tmp;
      }
      else
      {
          leaf_node it_leaf;
          it_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
          if(position==it_leaf.now_size-1)//最后一个结点
          {
              if(it_leaf.next!=0)//保证后面有结点
              {
                  offset=it_leaf.next;
                  position=0;
              }
              else position++;
          }
          else
              position++;
          return tmp;
      }
    }
    iterator& operator++() {
        if(*this==what_tree->end())//最后一个数据了，++没有意义了吧
        {
            what_tree= nullptr;
            offset=0;
            position=0;
            return *this;
        }
        else
        {
            leaf_node it_leaf;
            it_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
            if(position==it_leaf.now_size-1)//最后一个结点
            {
                if(it_leaf.next!=0)//保证后面有结点
                {
                    offset=it_leaf.next;
                    position=0;
                }
                else position++;
            }
            else
                position++;
            return *this;
        }
    }
    iterator operator--(int) {
      iterator tmp=*this;
      if(*this==what_tree->begin())//第一个数据
      {
          what_tree= nullptr;
          offset=0;
          position=0;
          return tmp;
      }
      else
      {
          leaf_node tmp_leaf;
          tmp_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
          if(position==0)//节点的第一个数据
          {
              offset=tmp_leaf.prev;
              if(offset==0)////////////////
                  position--;
              else{leaf_node q;
              q=what_tree->read_leaf(tmp_leaf.prev,sizeof(leaf_node));
              position=q.now_size-1;}
          }
          else//正常情况
              position--;

          return tmp;
      }

    }
    iterator& operator--() {
        if(*this==what_tree->begin())//第一个数据
        {
            what_tree= nullptr;
            offset=0;
            position=0;
            return *this;
        }
        else {
            leaf_node tmp_leaf;
            tmp_leaf = what_tree->read_leaf(offset, sizeof(leaf_node));
            if (position == 0)//节点的第一个数据
            {
                offset = tmp_leaf.prev;
                if (offset == 0)/////////////////
                    position--;
                else {
                    leaf_node q;
                    q = what_tree->read_leaf(tmp_leaf.prev, sizeof(leaf_node));
                    position = q.now_size - 1;
                }
            } else//正常情况
                position--;

            return *this;
        }
    }
    // Overloaded of operator '==' and '!='
    // Check whether the iterators are same
    value_type& operator*() const {////////////??????
      // Todo operator*, return the <K,V> of iterator
      leaf_node tmp;
      tmp=what_tree->read_leaf(offset,sizeof(leaf_node));
      return tmp.data[position].value;
    }
    bool operator==(const iterator& rhs) const {
      return (position==rhs.position&&offset==rhs.offset&&what_tree==rhs.what_tree);
    }
    bool operator==(const const_iterator& rhs) const {
        return (position==rhs.position&&offset==rhs.offset&&what_tree==rhs.what_tree);
    }
    bool operator!=(const iterator& rhs) const {
        return (position!=rhs.position||offset!=rhs.offset||what_tree!=rhs.what_tree);
    }
    bool operator!=(const const_iterator& rhs) const {
        return (position!=rhs.position||offset!=rhs.offset||what_tree!=rhs.what_tree);
    }
    value_type* operator->() const noexcept {
      /**
       * for the support of it->first.
       * See
       * <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/>
       * for help.
       */
       //要干啥？
    }
  };
  class const_iterator {
      friend class BTree;
    // it should has similar member method as iterator.
    //  and it should be able to construct from an iterator.
   private:
    // Your private members go here
    BTree *what_tree;
    offset_type offset;
    int position;
   public:
    const_iterator() {
      // TODO
      position=0;
      offset=0;
      what_tree= nullptr;
    }
    const_iterator(BTree *x,offset_type offs,int p)
    {
        what_tree=x;
        offset=offs;
        position=p;
    }
    const_iterator(const const_iterator& other) {
      // todo
      what_tree=other.what_tree;
      position=other.position;
      offset=other.offset;
    }
    const_iterator(const iterator& other) {
      // TODO
        what_tree=other.what_tree;
        position=other.position;
        offset=other.offset;
    }
    const_iterator operator++(int) {
          const_iterator tmp=*this;
          if(*this==what_tree->cend())//最后一个数据了，++没有意义了吧
          {
              what_tree= nullptr;
              offset=0;
              position=0;
              return tmp;
          }
          else
          {
              leaf_node it_leaf;
              it_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
              if(position==it_leaf.now_size-1)//最后一个结点
              {
                  if(it_leaf.next!=0)//保证后面有结点
                  {
                      offset=it_leaf.next;
                      position=0;
                  }
                  else position++;
              }
              else
                  position++;
              return tmp;
          }
      }
      const_iterator& operator++() {
          if(*this==what_tree->cend())//最后一个数据了，++没有意义了吧
          {
              what_tree= nullptr;
              offset=0;
              position=0;
              return *this;
          }
          else
          {
              leaf_node it_leaf;
              it_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
              if(position==it_leaf.now_size-1)//最后一个结点
              {
                  if(it_leaf.next!=0)//保证后面有结点
                  {
                      offset=it_leaf.next;
                      position=0;
                  }
                  else position++;
              }
              else
                  position++;
              return *this;
          }
      }
      const_iterator operator--(int) {
          iterator tmp=*this;
          if(*this==what_tree->cbegin())//第一个数据
          {
              what_tree= nullptr;
              offset=0;
              position=0;
              return tmp;
          }
          else
          {
              leaf_node tmp_leaf;
              tmp_leaf=what_tree->read_leaf(offset,sizeof(leaf_node));
              if(position==0)//节点的第一个数据
              {
                  offset=tmp_leaf.prev;
                  if(offset==0)////////////////
                      position--;
                  else{leaf_node q;
                      q=what_tree->read_leaf(tmp_leaf.prev,sizeof(leaf_node));
                      position=q.now_size-1;}
              }
              else//正常情况
                  position--;

              return tmp;
          }

      }
      const_iterator& operator--() {
          if(*this==what_tree->cbegin())//第一个数据
          {
              what_tree= nullptr;
              offset=0;
              position=0;
              return *this;
          }
          else {
              leaf_node tmp_leaf;
              tmp_leaf = what_tree->read_leaf(offset, sizeof(leaf_node));
              if (position == 0)//节点的第一个数据
              {
                  offset = tmp_leaf.prev;
                  if (offset == 0)/////////////////
                      position--;
                  else {
                      leaf_node q;
                      q = what_tree->read_leaf(tmp_leaf.prev, sizeof(leaf_node));
                      position = q.now_size - 1;
                  }
              } else//正常情况
                  position--;

              return *this;
          }
      }
      // Overloaded of operator '==' and '!='
      // Check whether the iterators are same
      value_type& operator*() const {////////////??????
          // Todo operator*, return the <K,V> of iterator
          leaf_node tmp;
          tmp=what_tree->read_leaf(offset,sizeof(leaf_node));
          return tmp.data[position].value;
      }
      bool operator==(const iterator& rhs) const {
          return (position==rhs.position&&offset==rhs.offset&&what_tree==rhs.what_tree);
      }
      bool operator==(const const_iterator& rhs) const {
          return (position==rhs.position&&offset==rhs.offset&&what_tree==rhs.what_tree);
      }
      bool operator!=(const iterator& rhs) const {
          return (position!=rhs.position||offset!=rhs.offset||what_tree!=rhs.what_tree);
      }
      bool operator!=(const const_iterator& rhs) const {
          return (position!=rhs.position||offset!=rhs.offset||what_tree!=rhs.what_tree);
      }
    // And other methods in iterator, please fill by yourself.
  };
  // Default Constructor and Copy Constructor
  BTree() {
    // Todo Default
    open_file();
    if(exists==0) build_bpt();
  }
  BTree(const BTree& other) {

    // Todo Copy
  }
  BTree& operator=(const BTree& other) {
    // Todo Assignment
  }
  ~BTree() {
      close_file();
    // Todo Destructor
  }

  void insert_node(const Key &key,offset_type offset,internal_node current_node)
  {
      int pos;
      for(pos=0;pos<current_node.now_size;pos++)
      {
          if(key<current_node.data[pos].key)
              break;
      }
      for(int i=current_node.now_size-1;i>=pos;i--)
      {current_node.data[i+1].key=current_node.data[i].key;
       current_node.data[i+1].offset=current_node.data[i].offset;
      }
      current_node.data[pos].offset=offset;
      current_node.data[pos].key=key;
      current_node.now_size++;
      if(current_node.now_size<=M)
          write_index(current_node.offset,sizeof(internal_node),current_node);
      else split_node(current_node);
  }

    pair<iterator,OperationResult> insert_leaf(const Key &key,const Value &value,leaf_node &current_leaf)
    {
        int pos;
        for(pos=0;pos<current_leaf.now_size;pos++)
        {
            if(key==current_leaf.data[pos].key)
                return pair<iterator,OperationResult > (iterator(nullptr),Fail);
            if(key<current_leaf.data[pos].key)
                break;
        }//找到添加的位置
        for(int i=current_leaf.now_size-1;i>=pos;i--)
        {
            current_leaf.data[i+1].key=current_leaf.data[i].key;
            current_leaf.data[i+1].value=current_leaf.data[i].value;
        }//后移
        current_leaf.now_size++;
        saving.num_of_data++;
        current_leaf.data[pos].value=value;
        current_leaf.data[pos].key=key;

        iterator it;
        it.offset=current_leaf.offset;
        it.position=pos;
        write_index(off,sizeof(save),saving);
        if(current_leaf.now_size<=L)
            write_leaf(current_leaf.offset,sizeof(leaf_node),current_leaf);
        else
            split_leaf(current_leaf,it,key);
        return pair<iterator,OperationResult> (it,Success);

    }

    void split_leaf(leaf_node current_leaf,iterator &it,Key key)
    {
      leaf_node new_leaf;
      new_leaf.now_size=current_leaf.now_size/2;
      current_leaf.now_size=current_leaf.now_size/2;
      it.offset=new_leaf.offset=saving.eend;
      saving.eend=saving.eed+new_leaf.offset;
      new_leaf.father=current_leaf.father;
      //移动
      for(int i=0;i<new_leaf.now_size;++i)
      {
          new_leaf.data[i].value=current_leaf.data[i+current_leaf.now_size].value;
          new_leaf.data[i].key=current_leaf.data[i+current_leaf.now_size].key;
          if(new_leaf.data[i].key==key)
          {
              it.offset=new_leaf.offset;
              it.position=i;
          }
      }

      //新节点和之前的相关叶子节点进行连接
      new_leaf.next=current_leaf.next;
      new_leaf.prev=current_leaf.offset;
      current_leaf.next=new_leaf.offset;//前驱后继的连接

      leaf_node next_leaf;
      if(new_leaf.next!=0)//后面有叶子,把后面的更新并写入
      {
          next_leaf=read_leaf(new_leaf.offset,sizeof(leaf_node));
          next_leaf.prev=new_leaf.offset;
          write_leaf(new_leaf.offset,sizeof(leaf_node),next_leaf);
      }

      //最后一个叶子了
      if(saving.last_leaf==current_leaf.offset)
          saving.last_leaf=new_leaf.offset;

      //写入
      write_leaf(new_leaf.offset,sizeof(leaf_node),new_leaf);
      write_leaf(current_leaf.offset,sizeof(leaf_node),current_leaf);
      write_index(off,sizeof(save),saving);

      //更新父亲
      internal_node father_node;
      father_node=read_index(current_leaf.father,sizeof(internal_node));
      insert_node(new_leaf.data[0].key,new_leaf.offset,father_node);

    }

    void split_node(internal_node &current_node)
    {
      internal_node new_node;
      new_node.now_size=current_node.now_size/2;
      current_node.now_size=current_node.now_size/2;
      new_node.type=current_node.type;
      new_node.offset=saving.eend;
      new_node.father=current_node.father;
      saving.eend+=sizeof(internal_node);
      //复制关键字
      for(int i=0;i<new_node.now_size;++i)
      {new_node.data[i].offset=current_node.data[i+current_node.now_size].offset;
       new_node.data[i].key=current_node.data[i+current_node.now_size].key;
      }

      //更新这个节点下的孩子的地址
      leaf_node tmp_leaf;
      internal_node tmp_node;
      for(int i=0;i<new_node.now_size;++i)//检查它的孩子是叶子还是内部节点
      {
          if(new_node.type==1)
          {//如果孩子是叶子结点，更新叶子节点的父亲，再写入
              tmp_leaf=read_leaf(new_node.data[i].offset,sizeof(leaf_node));
              tmp_leaf.father=new_node.offset;
              write_leaf(tmp_leaf.offset,sizeof(leaf_node),tmp_leaf);
          }
          else
          {//如果孩子不是叶子结点，更新内部结点的父亲，再写入
              tmp_node=read_index(new_node.data[i].offset,sizeof(internal_node));
              tmp_node.father=new_node.offset;
              write_index(tmp_node.offset,sizeof(internal_node),tmp_node);
          }
      }

      //结点是根的情况要单独考虑，重新生成一个根
      if(current_node.offset==saving.root)
      {
          internal_node new_root;
          new_root.offset=saving.eend;
          saving.eend+=sizeof(internal_node);
          new_root.father=0;
          new_root.type=0;
          new_root.now_size=2;
          new_root.data[0].key=current_node.data[0].key;
          new_root.data[1].key=current_node.data[1].key;
          new_root.data[0].offset=current_node.data[0].offset;
          new_root.data[1].offset=current_node.data[1].offset;

          new_node.father=new_root.offset;
          current_node.father=new_root.offset;//分裂的结点的父亲都是新的根

          saving.root=new_root.offset;

          //写入文件
          write_index(current_node.offset,sizeof(internal_node),current_node);
          write_index(new_node.offset,sizeof(internal_node),new_node);
          write_index(new_root.offset,sizeof(internal_node),new_root);
          write_index(off,sizeof(save),saving);

      }
      else//原来的结点不是根，正常写入，还要将这个插入父亲的结点里面
      {
          write_index(current_node.offset, sizeof(internal_node), current_node);
          write_index(new_node.offset, sizeof(internal_node), new_node);
          write_index(off, sizeof(save), saving);
          //插入父亲的结点中
          internal_node tmp_father;
          tmp_father=read_index(current_node.offset,sizeof(internal_node));
          insert_node(new_node.data[0].key,new_node.offset,tmp_father);
      }
    }


  // Insert: Insert certain Key-Value into the database
  // Return a pair, the first of the pair is the iterator point to the new
  // element, the second of the pair is Success if it is successfully inserted
  pair<iterator, OperationResult> insert(const Key& key, const Value& value) {

    offset_type leaf_offset=find_location(key,saving.root);
    leaf_node leaf;
    if(saving.num_of_data==0||leaf_offset==0)//第一个数据或者是第一个叶子
    {
        leaf=read_leaf(saving.head,sizeof(leaf_node));
        pair<iterator,OperationResult> p;
        p=insert_leaf(key,value,leaf);////
        if(p.second==Fail) return p;
        offset_type offset=leaf.father;
        internal_node node;
        while(offset!=0)
        {
            node=read_index(offset,sizeof(internal_node));
            node.data[0].key=key;
            write_index(offset,sizeof(internal_node),node);
            offset=node.father;
        }
        return p;
    }
    leaf=read_index(leaf_offset,sizeof(leaf_node));
    pair<iterator,OperationResult> p;
    p=insert_leaf(key,value,leaf);
    return p;
  }
  // Erase: Erase the Key-Value
  // Return Success if it is successfully erased
  // Return Fail if the key doesn't exist in the database
  OperationResult erase(const Key& key) {
    //TODO erase function
    return Fail;  // If you can't finish erase part, just remaining here.
  }
  // Return a iterator to the beginning
  iterator begin() {
      return iterator(this,saving.first_leaf,0);
  }
  const_iterator cbegin() const {
      return const_iterator(this,saving.first_leaf,0);
  }
  // Return a iterator to the end(the next element after the last)
  iterator end() {
      leaf_node last;
      last=read_leaf(saving.last_leaf,sizeof(leaf_node));
      int x=last.now_size;
      return iterator(this,saving.last_leaf,x);
  }
  const_iterator cend() const {
      leaf_node last;
      last=read_leaf(saving.last_leaf,sizeof(leaf_node));
      int x=last.now_size;
      return const_iterator(this,saving.last_leaf,x);
  }
  // Check whether this BTree is empty
  bool empty() const {
      return saving.num_of_data==0;
  }
  // Return the number of <K,V> pairs
  size_t size() const {
      return saving.num_of_data;
  }
  // Clear the BTree
  void clear() {
      ffile.open("bpt",std::ios::binary);
      ffile.close();
      open_file();
      build_bpt();
  }
  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key& key) const {
      return static_cast<size_t> (find(key)!=iterator(nullptr));
  }
  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is
   * returned.
   */
  iterator find(const Key& key) {
      offset_type leaf_off=find_location(key,saving.root);
      if(leaf_off==0)
          return end();
      leaf_node current_leaf;
      current_leaf=read_leaf(leaf_off,sizeof(leaf_node));
      int  i;
      for(i=0;i<current_leaf.now_size;++i)
      {
          if(key==current_leaf.data[i].key)
              return iterator(this,leaf_off,i);
      }
      if(i==current_leaf.now_size)
          return end();
  }
  const_iterator find(const Key& key) const {
      offset_type leaf_off=find_location(key,saving.root);
      if(leaf_off==0)
          return cend();
      leaf_node current_leaf;
      current_leaf=read_leaf(leaf_off,sizeof(leaf_node));
      int  i;
      for(i=0;i<current_leaf.now_size;++i)
      {
          if(key==current_leaf.data[i].key)
              return const_iterator(this,leaf_off,i);
      }
      if(i==current_leaf.now_size)
          return cend();
  }
};
}  // namespace sjtu
