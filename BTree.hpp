# include "utility.hpp"
# include <fstream>
# include <functional>
# include <cstddef>
# include "exception.hpp"

namespace sjtu {

    template <class Key, class Value, class Compare = std::less<Key> >
    class BTree {
    public:
        typedef pair <Key, Value> value_type;
        typedef ssize_t offset_type;
        class iterator;
        class const_iterator;

    private:
        static const int M = 1000; //非叶子结点的长度
        static const int L = 200;  //叶子结点的长度
        static const int off = 0;//saving的偏移量


        struct savee {
            offset_type first_leaf;          // 第一个节点
            offset_type last_leaf;          //最后一个
            offset_type root;          //树根啊
            size_t size;          // 总共的个数
            offset_type eend;         // end
            savee() {
                first_leaf = 0;
                last_leaf = 0;
                root = 0;
                size = 0;
                eend= 0;
            }
        };
        savee saving;//储存树的信息
        struct internalNode {
            offset_type offset;      	// 偏移量
            offset_type father;           	// 父亲
            offset_type data[M + 1];     	// 若干孩子
            Key key[M + 1];   	// 关键字
            int cnt;              	// 数据个数
            bool type;            	// bool为1——下面就是叶子。0——下面还是非叶子
            internalNode() {
                offset = 0,father = 0;
                for (int i = 0; i <= M; ++i) data[i] = 0;
                cnt = 0;
                type = 0;
            }
        };
        struct leafNode {
            offset_type offset;          // 偏移量
            offset_type father;               // 父亲
            offset_type pre, nxt;
            int cnt;                  // 多少个数据
            value_type data[L + 1];   // 数据
            leafNode() {
                offset = 0;
                father = 0;
                pre = 0;
                nxt = 0;
                cnt = 0;
            }
        };
        FILE *fp;
        bool fp_open;
        bool file_already_exists;

        inline void openFile() {
            file_already_exists = 1;
            if (fp_open == 0) {
                fp = fopen("bpt.txt", "rb+");
                if (fp == nullptr) {
                    file_already_exists = 0;
                    fp = fopen("bpt.txt", "w");
                    fclose(fp);
                    fp = fopen("bpt.txt", "rb+");
                } else read_file(&saving, off, 1, sizeof(savee));
                fp_open = 1;
            }
        }

        inline void read_file(void *ff, offset_type offset, size_t num, size_t size) const {
            if (fseek(fp, offset, SEEK_SET)) throw "failed!stupid!";
            fread(ff, size, num, fp);
        }

        inline void closeFile() {
            if (fp_open == 1) {
                fclose(fp);
                fp_open = 0;
            }
        }

        inline void write_file(void *ff, offset_type offset, size_t num, size_t size) const {
            if (fseek(fp, offset, SEEK_SET)) throw "failed!rubbish!";
            fwrite(ff, size, num, fp);
        }

         void build_bpt() {
            saving.size = 0;
            saving. eend = sizeof(savee);
            internalNode root;
            leafNode new_leaf;
            saving.root = root.offset = saving. eend;
            saving. eend += sizeof(internalNode);
            saving.first_leaf = saving.last_leaf = new_leaf.offset = saving. eend;
            saving. eend += sizeof(leafNode);
            root.type = 1;
            root.father = 0;
            root.cnt = 1;
            new_leaf.father = root.offset;
            root.data[0] = new_leaf.offset;
            new_leaf.nxt = new_leaf.pre = 0;
            new_leaf.cnt = 0;
            write_file(&saving, off, 1, sizeof(savee));
            write_file(&root, root.offset, 1, sizeof(internalNode));
            write_file(&new_leaf, new_leaf.offset, 1, sizeof(leafNode));
        }

        offset_type find_location(const Key &key, offset_type offset) {
            internalNode node;
            read_file(&node, offset, 1, sizeof(internalNode));
            if(node.type == 1) {//孩子是叶子节点
                int pos ;
                for (pos=0; pos < node.cnt; ++pos)
                    if (key < node.key[pos]) break;
                if (pos == 0) return 0;
                return node.data[pos - 1];
            } else {
                int pos;
                for (pos=0; pos < node.cnt; ++pos)
                    if (key < node.key[pos]) break;
                if (pos == 0) return 0;
                return find_location(key, node.data[pos - 1]);//递归查找
            }
        }


        pair <iterator, OperationResult> insert_leaf(leafNode &current_leaf, const Key &key, const Value &value) {

            int pos ;
            for (pos=0; pos < current_leaf.cnt; ++pos) {
                if (key == current_leaf.data[pos].first)
                    return pair <iterator, OperationResult> (iterator(nullptr), Fail);			// there are elements with the same key
                if (key < current_leaf.data[pos].first)
                    break;
            }//找到添加的位置
            for (int i = current_leaf.cnt - 1; i >= pos; --i)
            { current_leaf.data[i+1].first =current_leaf.data[i].first;
              current_leaf.data[i+1].second = current_leaf.data[i].second;
            }
            current_leaf.data[pos].first = key;
            current_leaf.data[pos].second = value;
            ++current_leaf.cnt;
            ++saving.size;

            iterator it;
            it.what_tree = this;
            it. position = pos;
            it.offset = current_leaf.offset;
            write_file(&saving, off, 1, sizeof(savee));
            if(current_leaf.cnt <= L) write_file(&current_leaf, current_leaf.offset, 1, sizeof(leafNode));
            else split_leaf(current_leaf, it, key);
            return pair <iterator, OperationResult> (it, Success);
        }


        void insert_node(internalNode &current_node, const Key &key, offset_type ch) {
            int pos ;
            for (pos=0; pos < current_node.cnt; ++pos)
                if (key < current_node.key[pos]) break;
            for (int i = current_node.cnt - 1; i >= pos; --i)
                current_node.key[i+1] = current_node.key[i];
            for (int i = current_node.cnt - 1; i >= pos; --i)
                current_node.data[i+1] =current_node.data[i];
            current_node.key[pos] = key;
            current_node.data[pos] = ch;
            ++current_node.cnt;
            if(current_node.cnt <= M)
                write_file(&current_node, current_node.offset, 1, sizeof(internalNode));
            else split_node(current_node);
        }


        void split_leaf(leafNode &current_leaf, iterator &it, const Key &key) {
            leafNode newleaf;
            newleaf.cnt = current_leaf.cnt - (current_leaf.cnt /2);
            current_leaf.cnt =current_leaf.cnt/2;
            newleaf.offset = saving. eend;
            saving. eend += sizeof(leafNode);
            newleaf.father =current_leaf.father;
            //move
            for (int i=0; i<newleaf.cnt; ++i) {
                newleaf.data[i].first = current_leaf.data[i +current_leaf.cnt].first;
                newleaf.data[i].second = current_leaf.data[i + current_leaf.cnt].second;
                if(newleaf.data[i].first == key) {
                    it.offset = newleaf.offset;
                    it. position = i;
                }
            }
            // 新结点和之前的 结点连接
            newleaf.nxt = current_leaf.nxt;
            newleaf.pre = current_leaf.offset;
            current_leaf.nxt = newleaf.offset;

            leafNode next_leaf;
            if(newleaf.nxt == 0) saving.last_leaf = newleaf.offset;
            else {//后面还有叶子，更新后面的
                read_file(&next_leaf, newleaf.nxt, 1, sizeof(leafNode));
                next_leaf.pre = newleaf.offset;
                write_file(&next_leaf, next_leaf.offset, 1, sizeof(leafNode));
            }
            //写入

            write_file(&current_leaf, current_leaf.offset, 1, sizeof(leafNode));
            write_file(&newleaf, newleaf.offset, 1, sizeof(leafNode));
            write_file(&saving, off, 1, sizeof(savee));

            // 更新父亲
            internalNode father_node;
            read_file(&father_node, current_leaf.father, 1, sizeof(internalNode));
            insert_node(father_node, newleaf.data[0].first, newleaf.offset);
        }


        void split_node(internalNode &current_node) {
            internalNode newnode;
            newnode.cnt = current_node.cnt - (current_node.cnt /2);
            current_node.cnt =current_node.cnt/2;
            newnode.type = current_node.type;
            newnode.father = current_node.father;
            newnode.offset = saving. eend;
            saving. eend += sizeof(internalNode);
            //复制关键字
            for (int i = 0; i < newnode.cnt; ++i)
            {newnode.key[i] = current_node.key[i + current_node.cnt];
             newnode.data[i] =current_node.data[i +current_node.cnt];
            }

            //更新这个结点下孩子的地址
            leafNode tmp_leaf;
            internalNode tmp_node;
            for (int i = 0; i < newnode.cnt; ++i) {
                if(newnode.type == 1) {  				// his child is leaf
                    read_file(&tmp_leaf, newnode.data[i], 1, sizeof(leafNode));
                    tmp_leaf.father = newnode.offset;
                    write_file(&tmp_leaf, tmp_leaf.offset, 1, sizeof(leafNode));
                } else {
                    read_file(& tmp_node, newnode.data[i], 1, sizeof(internalNode));
                    tmp_node.father = newnode.offset;
                    write_file(& tmp_node, tmp_node.offset, 1, sizeof(internalNode));
                }
            }

            //结点时根，需要重新生成根

            if(current_node.offset == saving.root) {
                internalNode new_root;
                new_root.offset =saving. eend;
                saving. eend += sizeof(internalNode);
                new_root.father = 0;
                new_root.type = 0;
                new_root.cnt = 2;
                new_root.key[0] = current_node.key[0];
                new_root.data[0] = current_node.offset;
                new_root.key[1] = newnode.key[0];
                new_root.data[1] = newnode.offset;

                current_node.father = new_root.offset;
                newnode.father = new_root.offset;
                saving.root = new_root.offset;//分裂的结点的父亲都是新根

                //写入
                write_file(&saving, off, 1, sizeof(savee));
                write_file(&current_node, current_node.offset, 1, sizeof(internalNode));
                write_file(&newnode, newnode.offset, 1, sizeof(internalNode));
                write_file(&new_root, new_root.offset, 1, sizeof(internalNode));
            } else {															// not root
                write_file(&saving, off, 1, sizeof(savee));
                write_file(&current_node, current_node.offset, 1, sizeof(internalNode));
                write_file(&newnode, newnode.offset, 1, sizeof(internalNode));

                internalNode tmp_father;
                read_file(&tmp_father, current_node.father, 1, sizeof(internalNode));
                insert_node(tmp_father, newnode.key[0], newnode.offset);
            }
        }



    public:
        class iterator {
            friend class BTree;
        private:
            offset_type offset;        // offset of the leaf node
            int position;							// place of the element in the leaf node
            BTree *what_tree;
        public:
            iterator() {
                what_tree = nullptr;
                position = 0;
                offset = 0;
            }
            iterator(BTree *what, offset_type offs = 0, int pos = 0) {
                what_tree = what;
                offset = offs;  position = pos;
            }
            iterator(const iterator& other) {
                what_tree = other.what_tree;
                offset = other.offset;
                position = other. position;
            }
            iterator(const const_iterator& other) {
                what_tree = other.what_tree;
                offset = other.offset;
                position = other. position;
            }

            // to get the value type pointed by iterator.
            Value getValue() {
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                return p.data[ position].second;
            }

            OperationResult modify(const Value& value) {

                return Success;
            }

            // Return a new iterator which points to the n-next elements
            iterator operator++(int) {
                iterator it = *this;
                // 末尾
                if(*this == what_tree -> end()) {
                    what_tree = nullptr;  position = 0; offset = 0;
                    return it;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == p.cnt - 1) {
                    if(p.nxt == 0) ++  position;
                    else {
                        offset = p.nxt;
                        position = 0;
                    }
                }
                else ++  position;
                return it;
            }
            iterator& operator++() {
                if(*this == what_tree -> end()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return *this;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == p.cnt - 1) {
                    if(p.nxt == 0) ++ position;
                    else {
                        offset = p.nxt;
                        position = 0;
                    }
                }
                else ++  position;
                return *this;
            }
            iterator operator--(int) {
                iterator it = *this;
                if(*this == what_tree -> begin()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return it;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == 0) {
                    offset = p.pre;
                    leafNode q;
                    what_tree -> readFile(&q, p.pre, 1, sizeof(leafNode));
                    position = q.cnt - 1;
                }
                else --  position;
                return it;
            }
            iterator& operator--() {
                if(*this == what_tree -> begin()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return *this;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == 0) {
                    offset = p.pre;
                    leafNode q;
                    what_tree -> read_file(&q, p.pre, 1, sizeof(leafNode));
                    position = q.cnt - 1;
                }
                else --  position;
                return *this;
            }
            bool operator==(const iterator& rhs) const {
                return offset == rhs.offset &&  position == rhs. position && what_tree == rhs.what_tree;
            }
            bool operator==(const const_iterator& rhs) const {
                return offset == rhs.offset &&  position == rhs. position && what_tree == rhs.what_tree;
            }
            bool operator!=(const iterator& rhs) const {
                return offset != rhs.offset ||  position != rhs. position || what_tree != rhs.what_tree;
            }
            bool operator!=(const const_iterator& rhs) const {
                return offset != rhs.offset ||  position != rhs. position || what_tree != rhs.what_tree;
            }
        };

        class const_iterator {
            friend class BTree;
        private:
            offset_type offset;        // offset of the leaf node
            int position;							// place of the element in the leaf node
            const BTree *what_tree;
        public:
            const_iterator() {
                what_tree = nullptr;
                position = 0, offset = 0;
            }
            const_iterator(const BTree *what,offset_type offs = 0, int pos = 0) {
                what_tree = what;
                offset = offs;  position = pos;
            }
            const_iterator(const iterator& other) {
                what_tree = other.what_tree;
                offset = other.offset;
                position = other. position;
            }
            const_iterator(const const_iterator& other) {
                what_tree = other.what_tree;
                offset = other.offset;
                position = other. position;
            }
            // to get the value type pointed by iterator.
            Value getValue() {
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                return p.data[ position].second;
            }
            // Return a new iterator which points to the n-next elements
            const_iterator operator++(int) {
                const_iterator it = *this;
                //末尾
                if(*this == what_tree -> cend()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return it;
                }
                leafNode p;
                what_tree-> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == p.cnt - 1) {
                    if(p.nxt == 0) ++ position;
                    else {
                        offset = p.nxt;
                        position = 0;
                    }
                } else ++  position;
                return it;
            }
            const_iterator& operator++() {
                if(*this == what_tree -> cend()) {
                    what_tree = nullptr;  position = 0; offset = 0;
                    return *this;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == p.cnt - 1) {
                    if(p.nxt == 0) ++  position;
                    else {
                        offset = p.nxt;
                        position = 0;
                    }
                } else ++  position;
                return *this;
            }
            const_iterator operator--(int) {
                const_iterator it = *this;
                if(*this == what_tree -> cbegin()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return it;
                }
                leafNode p, q;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position== 0) {
                    offset = p.pre;
                    what_tree -> read_file(&q, p.pre, 1, sizeof(leafNode));
                    position = q.cnt - 1;
                } else --  position;
                return it;
            }
            const_iterator& operator--() {
                if(*this == what_tree -> cbegin()) {
                    what_tree = nullptr;
                    position = 0;
                    offset = 0;
                    return *this;
                }
                leafNode p;
                what_tree -> read_file(&p, offset, 1, sizeof(leafNode));
                if( position == 0) {
                    offset = p.pre;
                    leafNode q;
                    what_tree -> read_file(&q, p.pre, 1, sizeof(leafNode));
                    position = q.cnt - 1;
                } else --  position;
                return *this;
            }
            bool operator==(const iterator& rhs) const {
                return offset == rhs.offset &&  position == rhs. position && what_tree == rhs.what_tree;
            }
            bool operator==(const const_iterator& rhs) const {
                return offset == rhs.offset &&  position == rhs. position && what_tree == rhs.what_tree;
            }
            bool operator!=(const iterator& rhs) const {
                return offset != rhs.offset || position != rhs. position|| what_tree != rhs.what_tree;
            }
            bool operator!=(const const_iterator& rhs) const {
                return offset != rhs.offset ||  position != rhs. position || what_tree != rhs.what_tree;
            }
        };

        // Default Constructor and Copy Constructor

        BTree() {
            fp = nullptr;
            openFile();
            if (file_already_exists == 0) build_bpt();
        }

        BTree(const BTree& other) {

        }

        BTree& operator=(const BTree& other) {

        }

        ~BTree() {
            closeFile();
        }

        /**
         * Insert: Insert certain Key-Value into the database
         * Return a pair, the first of the pair is the iterator point to the new
         * element, the second of the pair is Success if it is successfully inserted
         */
        pair <iterator, OperationResult> insert(const Key& key, const Value& value) {
            offset_type leaf_offset = find_location(key,saving.root);
            leafNode leaf;
            if(saving.size == 0 || leaf_offset == 0)//第一个数据或者第一个叶子
            {
                read_file(&leaf, saving.first_leaf, 1, sizeof(leafNode));
                pair <iterator, OperationResult> p= insert_leaf(leaf, key, value);
                if(p.second == Fail) return p;
                offset_type offset = leaf.father;
                internalNode node;
                while(offset != 0) {
                    read_file(&node, offset, 1, sizeof(internalNode));
                    node.key[0] = key;
                    write_file(&node, offset, 1, sizeof(internalNode));
                    offset = node.father;
                }
                return p;
            }
            read_file(&leaf, leaf_offset, 1, sizeof(leafNode));
            pair <iterator, OperationResult> p = insert_leaf(leaf, key, value);
            return p;
        }

        /**
         * Erase: Erase the Key-Value
         * Return Success if it is successfully erased
         * Return Fail if the key doesn't exist in the database
         */
        OperationResult erase(const Key& key) {
            return Success;
        }

        // Return a iterator to the beginning
        iterator begin() {
            return iterator(this, saving.first_leaf, 0);
        }
        const_iterator cbegin() const {
            return const_iterator(this, saving.first_leaf, 0);
        }
        // Return a iterator to the end(the next element after the last)
        iterator end() {
            leafNode last;
            read_file(&last, saving.last_leaf, 1, sizeof(leafNode));
            return iterator(this, saving.last_leaf, last.cnt);
        }
        const_iterator cend() const {
            leafNode last;
            read_file(&last, saving.last_leaf, 1, sizeof(leafNode));
            return const_iterator(this, saving.last_leaf, last.cnt);
        }
        // Check whether this BTree is empty
        bool empty() const
        {return saving.size == 0;}
        // Return the number of <K,V> pairs
        size_t size() const
        {return saving.size;}
        // Clear the BTree
        void clear() {
            fp = fopen("bpt.txt", "w");
            fclose(fp);
            openFile();
            build_bpt();
        }
        /**
         * Returns the number of elements with key
         *   that compares equivalent to the specified argument,
         * The default method of check the equivalence is !(a < b || b > a)
         */
        size_t count(const Key& key) const {
            return static_cast <size_t> (find(key) != iterator(nullptr));
        }
        Value at(const Key& key){
            iterator it = find(key);
            leafNode leaf;
            if(it == end()) {
                throw "fail!";
            }
            read_file(&leaf, it.offset, 1, sizeof(leafNode));
            return leaf.data[it.position].second;
        }
        /**
         * Finds an element with key equivalent to key.
         * key value of the element to search for.
         * Iterator to an element with key equivalent to key.
         *   If no such element is found, past-the-end (see end()) iterator is
         * returned.`
         */
        iterator find(const Key& key) {
            offset_type leaf_off = find_location(key, saving.root);
            if(leaf_off == 0) return end();
            leafNode current_leaf;
            read_file(&current_leaf, leaf_off, 1, sizeof(leafNode));
            for (int i = 0; i < current_leaf.cnt; ++i)
            {if (current_leaf.data[i].first == key)
                    return iterator(this, leaf_off, i);
            }
            return end();
        }
        const_iterator find(const Key& key) const {
            offset_type leaf_off = find_location(key);
            if(leaf_off == 0) return cend();
            leafNode current_leaf;
            read_file(&current_leaf, leaf_off, 1, sizeof(leafNode));
            for (int i = 0; i < current_leaf.cnt; ++i)
                if (current_leaf.data[i].first == key) return const_iterator(this, leaf_off, i);
            return cend();
        }
    };
}  // namespace sjtu
