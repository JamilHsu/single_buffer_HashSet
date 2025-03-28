#pragma once
#ifndef SINGLE_BUFFER_HASH_SET
#define SINGLE_BUFFER_HASH_SET
#include <vector>
#include <cassert>
#include <variant>
#include <cstdint>
#include <limits>
#include <algorithm>
#if _HAS_CXX20 || __cplusplus >= 202002L
template <
    class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    bool AutoRehash = true>
//Allocator is not supported
class single_buffer_hash_set {
public:
    using value_type = Key;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using size_type = std::uint32_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = Pred;
    using key_type = Key;

    using index = std::uint32_t;
    static constexpr index npos = static_cast<index>(-1);
    constexpr size_type max_size() const noexcept {
        return npos - 1;
    }
    static constexpr size_type prime_buckets[] = {
        53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
        49157, 98317, 196613, 393241, 786433, 1572869, 3145739,
        6291469, 12582917, 25165843, 50331653, 100663319, 201326611,
        402653189, 805306457, 1610612741, 3221225473, 4294967291
    };
    constexpr size_type max_bucket_count() const noexcept {
        return std::end(prime_buckets)[-1];
    }
private:
    // �ȨѤ����ϥΪ����URAII���O�A��std::vector�`�٤@�ӫ���
    // �򥻤W�N�O�ӥ\��R��A���]�tcapacity��std::vector
    // resize�ɪ��������ơA�]���u����rehash�ɤ~�|�I�sresize�A�Ӧ����¸�Ƥw�L�γB
    // ���F���ƨϥζ�R�Amax_load_factor�]�Q��i�F�o��
    class _index_vector {
    private:
        index* _ptr;
        index _size;
    public:
        [[no_unique_address]] [[msvc::no_unique_address]]
            std::conditional_t<AutoRehash, float, std::monostate>
                max_load_factor;
            //���ҥH�g�o��@�j��Ӥ��O²�檺float max_load_factor�O���F�b32�줸�պA�B���ϥ�AutoRehash�ɴ��4�줸�աA���ާڬ۷��h�æp���������n��
            index& operator[](index Pos) noexcept {
                return _ptr[Pos];
            }
            index operator[](index Pos) const noexcept {
                return _ptr[Pos];
            }
            auto size() const noexcept {
                return _size;
            }
            index* begin() const noexcept {
                return _ptr;
            }
            index* end() const noexcept {
                return _ptr + _size;
            }
            //�߱�Ҧ���ƨç��ܤj�p�A�]���S���O�s�즳��ƪ��ݨD
            void resize(size_type newSize) {
                index* temp = newSize ? new index[newSize] : nullptr;
                // �S����l�Ƹ�ƪ����n
                delete[] _ptr;
                _ptr = temp;
                _size = newSize;
            }
            void clear() noexcept {
                delete[] _ptr;
                _ptr = nullptr;
                _size = 0;
            }
            _index_vector() = delete;
            explicit _index_vector(size_type Size) :
                _ptr(Size ? new index[Size] : nullptr),
                _size(Size) {
                if constexpr (AutoRehash) {
                    max_load_factor = 1.0;
                }
            }
            explicit _index_vector(size_type Size, index Val) :
                _ptr(Size ? new index[Size] : nullptr),
                _size(Size) {
                std::fill(_ptr, _ptr + _size, Val);
                if constexpr (AutoRehash) {
                    max_load_factor = 1.0;
                }
            }
            _index_vector(const _index_vector& other) :
                _ptr(other._size ? new index[other._size] : nullptr),
                _size(other._size),
                max_load_factor(other.max_load_factor) {
                std::copy(other.begin(), other.end(), this->begin());
            }
            _index_vector(_index_vector&& other) noexcept :
                _ptr(other._ptr),
                _size(other._size),
                max_load_factor(other.max_load_factor) {
                other._ptr = nullptr;
                other._size = 0;
            }
            _index_vector& operator=(const _index_vector& other) {
                if (this->size() == other.size()) {
                    std::copy(other.begin(), other.end(), this->begin());
                }
                else {
                    //���e������w�ư��]�ۧڽ�Ȧӵo�Ϳ��~���i��
                    index* temp = other.size() ? new index[other.size()] : nullptr;//���i����t�A�P���ª���H�T�O���`�w��
                    delete[] _ptr;
                    _ptr = temp;
                    _size = other._size;
                    std::copy(other.begin(), other.end(), this->begin());
                }
                max_load_factor = other.max_load_factor;
                return *this;
            }
            _index_vector& operator=(_index_vector&& other) noexcept {
                if (this != &other) {
                    delete[] _ptr;
                    _ptr = other._ptr;
                    _size = other._size;
                    other._ptr = nullptr;
                    other._size = 0;
                    max_load_factor = other.max_load_factor;
                }
                return *this;
            }
            ~_index_vector() noexcept {
                delete[] _ptr;
            }
    };
    //������`�I
    struct Node {
        value_type payload;
        index next;//���V�U�@�Ӹ`�I������ 
        //�z�L�ϥί��ަӤ��Opointer�i�H�T�O�Y�K���h�w�İϤw���ʡA�]���έ��s�վ�next
        //�åBindex�b64�줸�Ҧ��U�ϥΤ�pointer�֤@�b���O����
        Node(const value_type& value, index Next)
            noexcept(noexcept(value_type(std::declval<const value_type&>())))
            :payload(value), next(Next) {
        }
        Node(value_type&& value, index Next)
            noexcept(noexcept(value_type(std::declval<value_type&&>())))
            :payload(std::move(value)), next(Next) {
        }
        //���غc��Ƥ��\�ϥ�payload���غc��ưѼƫغcNode�A��emplace�ϥ�
#pragma warning(push)
#pragma warning(disable: 26495)
//���غc��ƥ���l�Ʀ����ܼ�next�A���L�ѩ�Node�O�p�����c�A�ӨC�өI�s���غc��ƪ��a�賣���b�����l��next�A�ξP��Node�A�G�]���S�w���Ρu����l�Ʀ����v��ĵ�i
//�ߤ@�ϥΦ��غc��ƪ��a��O_index_emplace
        template <typename... Args>
        explicit Node(Args&&... args)
            noexcept(noexcept(value_type(std::forward<Args>(args)...)))
            :payload(std::forward<Args>(args)...) {
        }
#pragma warning(pop)
    };
    // ���O��Ʀ���
    std::vector<Node> forward_lists; //�ѳ�@�w�İϩҺc������V�쵲��C
    // �x�s���V�U�ӱ�}�Y������
    // �������~�A���F���ƨϥζ�R�Amax_load_factor�]�Q��i�F_index_vector�̭�
    _index_vector buckets;
    // [[no_unique_address]]�ثe�bMSVC�W�S���ĪG
    [[no_unique_address]] [[msvc::no_unique_address]] hasher hash;     // ���ƨ��
    [[no_unique_address]] [[msvc::no_unique_address]] key_equal equal; // �������
public:
    class const_iterator {
        std::vector<Node>::const_iterator _iter;
    public:
        using iterator_concept = std::random_access_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = typename single_buffer_hash_set::value_type;
        using difference_type = std::ptrdiff_t;
        using distance_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;
        const_iterator(std::vector<Node>::const_iterator it) noexcept :_iter(it) {};
        const_iterator() noexcept {};
        [[nodiscard]] const_reference operator*() const noexcept {
            return _iter->payload;
        }
        [[nodiscard]] const_pointer operator->() const noexcept {
            return std::addressof(_iter->payload);
        }
        [[nodiscard]] const_reference operator[](const difference_type offset) const noexcept {
            return _iter[offset].payload;
        }
        const_iterator& operator++() noexcept {
            ++_iter;
            return *this;
        }
        const_iterator operator++(int) noexcept {
            const_iterator temp = *this;
            ++*this;
            return temp;
        }
        const_iterator& operator--() noexcept {
            --_iter;
            return *this;
        }
        const_iterator operator--(int) noexcept {
            const_iterator temp = *this;
            --*this;
            return temp;
        }
        const_iterator& operator+=(const difference_type offset) noexcept {
            _iter += offset;
            return *this;
        }
        [[nodiscard]] const_iterator operator+(const difference_type offset) const noexcept {
            return const_iterator(_iter + offset);
        }
        [[nodiscard]] friend const_iterator operator+(
            const difference_type offset, const_iterator right) noexcept {
            return right + offset;
        }
        const_iterator& operator-=(const difference_type offset) noexcept {
            _iter -= offset;
            return *this;
        }
        [[nodiscard]] const_iterator operator-(const difference_type offset) const noexcept {
            return const_iterator(_iter - offset);
        }
        [[nodiscard]] difference_type operator-(const const_iterator& other) const noexcept {
            return _iter - other._iter;
        }
        [[nodiscard]] bool operator==(const const_iterator&) const noexcept = default;
        [[nodiscard]] auto operator<=>(const const_iterator&) const noexcept = default;
    };
    static_assert(std::random_access_iterator<const_iterator>);
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;

    [[nodiscard]] const_iterator begin() const noexcept {
        return const_iterator(forward_lists.begin());
    }
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }
    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator(forward_lists.end());
    }
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    [[nodiscard]] const_reverse_iterator rcbegin() const noexcept {
        return rbegin();
    }
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    [[nodiscard]] const_reverse_iterator rcend() const noexcept {
        return rend();
    }

    // �غc�禡
    // �۰ʺ޲z��
    explicit single_buffer_hash_set(size_type bucket_count = 53, size_type initialCapacity = 0, const hasher& hashFunction = hasher(), const key_equal& equalFunction = key_equal())
        requires AutoRehash
    : buckets(0), hash(hashFunction), equal(equalFunction) {
        forward_lists.reserve(initialCapacity);
        rehash(bucket_count);
    }
    // ��ʺ޲z��A�����b�غc�禡�����X��l��ƶq
    // The initial number of buckets must be given explicitly
    explicit single_buffer_hash_set(size_type bucket_count, size_type initialCapacity = 0, const hasher& hashFunction = hasher(), const key_equal& equalFunction = key_equal())
        requires (!AutoRehash)
    : buckets(bucket_count, npos), hash(hashFunction), equal(equalFunction) {
        forward_lists.reserve(initialCapacity);
    }

    // �p�⫢�Ʊ����
    [[nodiscard]] size_type bucket(const key_type& value) const
        noexcept(noexcept(hash(std::declval<key_type>()))) {
        assert(buckets.size() > 0);
        return hash(value) % buckets.size();
    }
    [[nodiscard]] size_type bucket_count() const noexcept {
        return buckets.size();
    }
    [[nodiscard]] size_type bucket_size(size_type nbucket) const noexcept {
        assert(nbucket < size());
        index now = buckets[nbucket];
        size_type i = 0;
        while (now != npos) {
            ++i;
            now = forward_lists[now].next;
        }
        return i;
    }
    [[nodiscard]] float load_factor() const noexcept {
        return float(size()) / (float)bucket_count();
    }
    [[nodiscard]] float max_load_factor() const noexcept {
        if constexpr (AutoRehash) {
            return buckets.max_load_factor;
        }
        else {
            return std::numeric_limits<float>::infinity();
        }
    };
    void max_load_factor(float factor) noexcept
        requires AutoRehash {
        buckets.max_load_factor = factor;
    }
    [[nodiscard]] size_type size() const noexcept {
        return static_cast<size_type>(forward_lists.size());
    }
    [[nodiscard]] bool empty() const noexcept {
        return forward_lists.empty();
    }
    void rehash(size_type nbuckets) {
        if constexpr (AutoRehash) {
            // don't violate a.bucket_count() >= a.size() / a.max_load_factor() invariant:
            nbuckets = (std::max)(nbuckets, static_cast<size_type>(size() / max_load_factor()));
            if (nbuckets <= bucket_count()) {
                return;// we already have enough buckets; nothing to do
            }
            //�u�ʷj�M���U�@�ӽ��(�]�d��p�G���ĥΤG���j)
            for (auto i : prime_buckets) {
                if (i >= nbuckets) {
                    nbuckets = i;
                    break;
                }
            }
        }
        buckets.resize(nbuckets);
        if constexpr (!AutoRehash) {
            if (nbuckets == 0) {
                return;
                //�b�����p�U�Ҧ��������ѦҤ��Ҧ��ġA���ȯ�z�L���N���i��s��
                //�����\�I�s����d��/���J/������ơArehash�Pclear�H��destroy_container���~
            }
        }
        std::fill(buckets.begin(), buckets.end(), npos);
        size_type i = 0;
        for (auto& node : forward_lists) {
            index temp = bucket(node.payload);
            node.next = buckets[temp];
            buckets[temp] = i;
            ++i;
        }
    }
    //�w���O�d�Ω�s�񤸯����O����
    //AutoRehash�ɡA�]�|�@�ּW�[���
    void reserve(size_type newcapacity) {
        forward_lists.reserve(newcapacity);
        if constexpr (AutoRehash) {
            rehash(static_cast<size_type>((newcapacity + 8) / max_load_factor()));
        }
    }
    size_type capacity() const noexcept {
        return static_cast<size_type>(forward_lists.capacity());
    }
    void clear() noexcept {
        forward_lists.clear();
        std::fill(buckets.begin(), buckets.end(), npos);
    }
    void shrink_to_fit() {
        forward_lists.shrink_to_fit();
    }
    //����e���ϥΪ��Ҧ��귽�A�Ϯe���B�󤣥i�ϥΦӥi�B��R�c��ƪ����A�C
    void destroy_container() noexcept {
        forward_lists.clear();
        forward_lists.shrink_to_fit();
        buckets.clear();
    }

    [[nodiscard]] index _index_find(const key_type& keyval)
        const noexcept(noexcept(_index_find_impl(std::declval<key_type>(), 0))) {
        return _index_find_impl(keyval, bucket(keyval));
    }
    [[nodiscard]] const_iterator find(const key_type& keyval)
        const noexcept(noexcept(_index_find(std::declval<key_type>()))) {
        index temp = _index_find(keyval);
        return temp == npos ?
            end() :
            begin() + temp;
    }
    [[nodiscard]] size_type count(const key_type& key)
        const noexcept(noexcept(_index_find(std::declval<value_type>()))) {
        return _index_find(key) != npos;
    }
    [[nodiscard]] bool contains(const key_type& key)
        const noexcept(noexcept(count(std::declval<value_type>()))) {
        return count(key);
    }

    std::pair<index, bool> _index_insert(const value_type& value) {
        return _index_insert_impl(value);
    }
    std::pair<index, bool> _index_insert(value_type&& value) {
        return _index_insert_impl(std::move(value));
    }
    std::pair<const_iterator, bool> insert(const value_type& value) {
        auto temp = _index_insert(value);
        return { begin() + temp.first,temp.second };
    }
    std::pair<const_iterator, bool> insert(value_type&& value) {
        auto temp = _index_insert(std::move(value));
        return { begin() + temp.first,temp.second };
    }
    template <class... Args>
    std::pair<index, bool> _index_emplace(Args&&... args) {
        if constexpr (AutoRehash) {
            _check_rehash_required_1();
        }
        // �������غc�����~��p��䫢�ƭ�
        forward_lists.emplace_back(std::forward<Args>(args)...);
        index bucket_index = bucket(forward_lists.back().payload);
        index exist_element = _index_find_impl(forward_lists.back().payload, bucket_index);
        if (exist_element == npos) {
            forward_lists.back().next = buckets[bucket_index];
            return { (buckets[bucket_index] = forward_lists.size() - 1), true };
        }
        else {
            forward_lists.pop_back();
            return { exist_element, false };
        }
    }
    template <class... Args>
    std::pair<const_iterator, bool> emplace(Args&&... args) {
        auto temp = _index_emplace(std::forward<Args>(args)...);
        return { begin() + temp.first,temp.second };
    }

    size_type erase(const key_type& value)
        noexcept(noexcept(_erase_and_relink(0, 0, 0))
            && noexcept(equal(std::declval<key_type>(), std::declval<key_type>()))) {
        index bucket_index = bucket(value);
        index list_index = buckets[bucket_index];
        index before = npos;
        while (list_index != npos) {
            if (equal(value, forward_lists[list_index].payload)) {
                _erase_and_relink(list_index, bucket_index, before);
                return 1;
            }
            before = list_index;
            list_index = forward_lists[list_index].next;
        }
        return 0;
    }
    void _index_erase(index Where)
        noexcept(noexcept(_erase_and_relink(0, 0, 0))) {
        assert(Where < size());
        index bucket_index = bucket(forward_lists[Where].payload);
        index before = _find_before(Where, bucket_index);
        _erase_and_relink(Where, bucket_index, before);
    }
    void erase(const_iterator Where)
        noexcept(noexcept(_index_erase(0))) {
        _index_erase(static_cast<index>(Where - begin()));
    }

    [[nodiscard]] hasher hash_function() const noexcept(noexcept(hasher(hash))) {
        return hash;
    }
    [[nodiscard]] key_equal key_eq() const noexcept(noexcept(key_equal(equal))) {
        return equal;
    }
private:
    void _check_rehash_required_1() {
        if (static_cast<float>(size() + 1) / bucket_count() > max_load_factor()) {
            rehash(static_cast<size_type>((size() + 8) / max_load_factor()));
        }
    }
    index _index_find_impl(const value_type& value, index bucket_index)
        const noexcept(noexcept(bucket(std::declval<key_type>())) && noexcept(equal(std::declval<key_type>(), std::declval<key_type>()))) {
        index list_index = buckets[bucket_index];
        while (list_index != npos) {
            if (equal(value, forward_lists[list_index].payload)) {
                return list_index;
            }
            list_index = forward_lists[list_index].next;
        }
        return npos;
    }
    template <class LRvalue_type>
    std::pair<index, bool> _index_insert_impl(LRvalue_type&& value) {
        if constexpr (AutoRehash) {
            _check_rehash_required_1();
        }
        index bucket_index = bucket(value);
        index exist_element = _index_find_impl(value, bucket_index);
        if (exist_element == npos) {
            //�N�s�������J�ܱ��}�Y
            forward_lists.emplace_back(std::forward<LRvalue_type>(value), buckets[bucket_index]);
            return { (buckets[bucket_index] = static_cast<index>(forward_lists.size() - 1)), true };
        }
        else {
            return { exist_element ,false };
        }
    }
    index _find_before(index Where, index bucket_index)
        const noexcept {
        index before = npos;
        index now = buckets[bucket_index];
        while (now != Where) {
            before = now;
            now = forward_lists[now].next;
        }
        return before;
    }
    //�����ؼФ����åγ̫�@�Ӥ�����ɪů�
    void _erase_and_relink(index Where, index bucket_index, index before)
        noexcept(noexcept(std::declval<value_type&>() = std::declval<value_type&&>())
            && noexcept(bucket(std::declval<value_type>()))) {
        //������e�@���s����U�@��
        /*if (before == npos) {
            buckets[bucket_index] = forward_lists[Where].next;
        }
        else {
            forward_lists[before].next = forward_lists[Where].next;
        }*/
        (before == npos ?
            buckets[bucket_index] :
            forward_lists[before].next)
            = forward_lists[Where].next;
        //���������������������̫�@��������
        if (Where != size() - 1) {
            index bucket_of_back = bucket(forward_lists.back().payload);
            index before_back = _find_before(size() - 1, bucket_of_back);
            //���ʳ̫�@�Ӥ�����ůʳB
            /*if (before_back == npos) {
                buckets[bucket_of_back] = Where;
            }
            else {
                forward_lists[before_back].next = Where;
            }*/
            (before_back == npos ?
                buckets[bucket_of_back] :
                forward_lists[before_back].next)
                = Where;
            forward_lists[Where] = std::move(forward_lists.back());
        }
        forward_lists.pop_back();
    }
};
#else
#error C++20 or later required
#endif // _HAS_CXX20
#endif // SINGLE_BUFFER_HASH_SET