# single_buffer_hash_set
基於單一緩衝區，separate chaining的高效hash set，使用遠比std::unordered_set少的記憶體。  
An efficient hash set based on a single buffer and separate chaining, using far less memory than std::unordered_set.

std::unordered_set為每個元素都獨立動態分配一塊記憶體，導致在元素較小時非常浪費空間。即便可以透過自訂分配器來彌補這一點，但良好的分配器並不容易撰寫。  
std::unordered_set dynamically allocates a piece of memory for each element independently, resulting in a lot of space waste when the elements are small. Even though it is possible to compensate for this with a custom allocator, good allocators are not easy to write.

此容器透過犧牲迭代器與參考的穩定性，將所有的元素儲存在一個std::vector中，因此不像std::unordered_set使用的list有巨大的空間開銷，不但減少了記憶體碎片化並減少記憶體使用量，還因為對快取友善以及大幅減少動態分配次數而顯著提高了效能。  
This container stores all elements in a std::vector, sacrificing the stability of iterators and references. Therefore, it does not have huge space overhead like the list used by std::unordered_set. This approach not only reduces memory fragmentation and overall memory usage but also significantly improves performance by being cache-friendly and minimizing the number of dynamic allocations.

以下是我在leetcode中的某個問題中，在使用完全相同的演算法與程式碼下，對於`std::unordered_set`和`single_buffer_hash_set`執行結果的比較:  
The following is a comparison of the execution results of `std::unordered_set` and `single_buffer_hash_set` using the exact same algorithm and code in a question in leetcode:  
|     |Runtime|Memory|
|-----|-------|------|
|std::unordered_set|575 ms|185.8 MB|
|single_buffer_hash_set|98 ms|19.3 ms|

#### 此容器的優點:  
元素在底層是連續儲存的，因此記憶體使用效率高，快取效能高  
在64位元組態下，桶所需的空間是指標的一半。這意味著，在同樣的空間下，可以獲得兩倍數量的桶!  
如果不曾移除過元素，則保證元素的排列順序與插入順序相同  
迭代速度與std::vector相當  

#### 此容器的缺點:  
儲存的元素類型必須是可移動的  
如果在插入元素時儲存空間不足，則所有迭代器與引用皆會失效。  
移除元素時，除了指向該元素的迭代器與參考之外，還會使指向最後一個元素的迭代器與參考失效。  
不支援自訂分配器  
雖然有經過測試，但仍不保證此容器沒有錯誤  

#### Advantages of this container:  
Elements are stored contiguously at the underlying layer, resulting in efficient memory usage and high cache performance.  
In a 64-bit system, each bucket requires only half the space of a pointer. This means that, with the same amount of memory, you can store twice as many buckets!    
If no elements have been removed, the elements are guaranteed to be in the same order as they were inserted.  
Iteration speed is comparable to std::vector.  

#### Disadvantages of this container:  
The element type must be movable.  
If there is insufficient space when inserting an element, all iterators and references will be invalidated.  
When an element is removed, iterators and references to that element, as well as those pointing to the last element, will be invalidated.  
Custom allocators are not supported.  
Although it has been tested, this container is not guaranteed to be free of errors.  


# 功能介紹/Function Introduction
使用方法與std::unordered_set類似，但是***具有不同的迭代器失效規則***，並且不支援部分函數與多載。  
The usage is similar to std::unordered_set, but ***has different iterator invalidation rules***, and does not support some functions and some overloads.  

在以下介紹中，沒有詳細說明的項目皆與std::unordered_set相同，但是可能缺少部分多載。  
In the following introduction, functions not described in detail are the same as std::unordered_set, but some overloads may be missing. 

## Syntax
```C++
template <
    class Key,
    class Hash = std::hash<Key>,
    class Pred = std::equal_to<Key>,
    bool AutoRehash = true>
    //Allocator is not supported
class single_buffer_hash_set;
```

### Parameters
`Key`  
The key type.

`Hash`  
The hash function object type.

`Pred`  
The equality comparison function object type.

`AutoRehash`  
此容器遵照C++的優良傳統，<del>讓你可以搬起石頭砸自己的腳</del>讓你不用為未使用的功能付費。如果為false，max_load_factor被將被固定為無限大，並跳過所有相關檢查。
使用者將可以*完全*控制桶的數量，相對的，控制負載因子不會過大就成為了使用者的責任。使用者甚至可以將桶數設為0，此時所有元素的參考仍皆有效，可以進行迭代，但不允許呼叫任何插入/搜尋/移除函式，***否則會觸發未定義的行為***。  
This container follows the C++ philosophy of <del>allowing you to shoot yourself in the foot</del> not paying for what you don't use. If `AutoRehash` is false, max_load_factor is fixed to infinity and all related checks are skipped.
The user will have full control over the number of buckets, but it is their responsibility to ensure that the load factor does not become too large. The user can even set the number of buckets to 0, in which case all references to elements are still valid and can be iterated, but calling any insertion/search/removal functions is not allowed, otherwise undefined behavior will be triggered.  

## Members
### Typedefs
#### Same as std::unordered_set
const_pointer, const_reference, difference_type, hasher, key_equal, key_type, pointer, reference, size_type, value_type

#### Different from std::unordered_set
`iterator`/`const_iterator`  
隨機存取唯讀迭代器。如果不曾移除過元素，則保證迭代順序與插入順序相同。  
Random access read-only iterator. If no elements are ever removed, the iteration order is guaranteed to be the same as the insertion order.

`reverse_iterator`/`const_reverse_iterator`  
= std::reverse_iterator<const_iterator>

#### Non-standard
`index`  
用於存取元素的索引。與迭代器和參考不同，即便發生了記憶體重新配置，索引也依然能夠存取相同的元素。
不過，索引也並非總是穩定的。每次移除元素時，除了目標元素之外，還會導致指向最後一個元素的索引無效，這點尤其要注意。  
使用索引存取元素的方法是搭配`begin()`一起使用：`.begin()[index]`  
An index for accessing elements. Unlike iterators and references, an index remains valid even after a memory reallocation, allowing access to the same element.  
However, an index is not always stable. When an element is removed, in addition to invalidating the index pointing to that element, it also invalidates the index pointing to the last element. This behavior should be carefully considered.  
To access elements by index, use `.begin()[index]`.

#### Do not exist:
allocator_type const_local_iterator local_iterator

### Functions
#### Standard
|Name | Description|
|-----|------------|
|begin | Designates the beginning of the controlled sequence.|
|bucket | Gets the bucket number for a key value.|
|bucket_count | Gets the number of buckets.|
|bucket_size | Gets the size of a bucket.|
|cbegin | Designates the beginning of the controlled sequence.|
|cend | Designates the end of the controlled sequence.|
|clear | Removes all elements.|
|contains | Check if there's an element with the specified key in the unordered_set.|
|count | Finds the number of elements matching a specified key.|
|emplace | Adds an element constructed in place.|
|empty | Tests whether no elements are present.|
|end | Designates the end of the controlled sequence.|
|erase | Removes elements at specified positions.|
|find | Finds an element that matches a specified key.|
|hash_function | Gets the stored hash function object.|
|insert | Adds elements.|
|key_eq | Gets the stored comparison function object.|
|load_factor | Counts the average elements per bucket.|
|max_bucket_count | Gets the maximum number of buckets.|
|max_load_factor | Gets or sets the maximum elements per bucket.|
|max_size | Gets the maximum size of the controlled sequence.|
|rehash | Rebuilds the hash table.|
|reserve | 🔴Reserve enough space to contain at least *n* elements without causing reference invalidation or rehashing.|
|size | Counts the number of elements.|
|single_buffer_hash_set | Constructs a container object.|

#### Non-standard
|Name | Description|
|-----|------------|
|capacity | Returns the number of elements that the container could contain without allocating more storage.|
|destroy_container | Empty the container and release all resources, including the hash table.|
|shrink_to_fit | Discard excess capacity. Do not reduce bucket_count.|
|nops | An unsigned integral value initialized to -1 that indicates "not found" when a search function fails.|
|_index_emplace | Adds an element constructed in place.|
|_index_erase | Removes elements at specified positions.|
|_index_find | Finds an element that matches a specified key.|
|_index_insert | Adds elements.|

#### Not supported
emplace_hint extract equal_range get_allocator insert_range merge swap(You can use std::swap directly)

### Operators
|Name | Description|
|-----|------------|
|single_buffer_hash_set::operator= | Copies a hash table.|

## Remarks
所有以_index_開頭的函數與沒有此前綴的對應函數的區別在於，前者使用`index`而不是迭代器作為參數和回傳值。  
在內部，所有透過迭代器進行操作的函數實際上僅是`_index_`系列函數的包裝器。  
至於為什麼要以`_`底線開頭?其實沒有什麼特別的原因。只是因為標準函式大多採用迭代器而不是索引，故特此加上`_`底線以示區別。放心，以底線開頭後接小寫字母的標識符僅在全域命名空間中被保留。

All functions prefixed with `_index_` differ from their non-prefixed counterparts in that they use an `index` instead of an `iterator` as both parameters and return values.  
Internally, all functions that operate using iterators are merely wrappers around the `_index_` function series.  
As for why these functions are prefixed with an underscore (`_`)? There is no special reason. Since standard functions typically use iterators rather than indices, the underscore is added to differentiate them. Rest assured, identifiers starting with an underscore followed by a lowercase letter are reserved only within the global namespace.

## `single_buffer_hash_set`
Constructs a container object.
```C++
explicit single_buffer_hash_set(size_type bucket_count = 53, size_type initialCapacity = 0, const hasher& hashFunction = hasher(), const key_equal& equalFunction = key_equal())
    requires AutoRehash;

// The initial number of buckets must be given explicitly
explicit single_buffer_hash_set(size_type bucket_count, size_type initialCapacity = 0, const hasher& hashFunction = hasher(), const key_equal& equalFunction = key_equal())
    requires (!AutoRehash);

single_buffer_hash_set(const single_buffer_hash_set& Right);

single_buffer_hash_set(single_buffer_hash_set&& Right);
```
### Parameters
`bucket_count`  
The minimum number of buckets.  
If `AutoRehash` is false, the exact number of buckets.

`initialCapacity`  
The minimum buffer size allocated for storing elements.

`hashFunction`  
The hash function object to store.

`equalFunction`  
The comparison function object to store.

`Right`  
The container to copy.

### Remarks
如果AutoRehash為false，則必須在建構函式中明確提供初始桶數量。初始桶數量可以為0，前提是在呼叫任何插入/搜尋/移除函式之前，必須明確呼叫rehash提供大於0的桶數。  
If `AutoRehash` is false, you must specify the initial number of buckets in the constructor. The number of buckets can be set to 0, but before calling any insert, search, or erase functions, `rehash` must be explicitly called with a non-zero bucket count.

## `rehash`
Rebuilds the hash table.
```C++
void rehash(size_type nbuckets);
```
### Parameters
`nbuckets`  
The requested number of buckets.


If `AutoRehash` is true, the member function alters the number of buckets to be at least `nbuckets` and rebuilds the hash table as needed.  
If `AutoRehash` is false, the function sets the number of buckets to exactly `nbuckets` and rebuilds the hash table.  
If `AutoRehash` is false and `nbuckets` is 0, the member function will destroy the hash table.

## `reserve`
Reserve enough space  to contain at least *n* elements without causing reference invalidation or rehashing.
```C++
void reserve(size_type n);
```
### Parameters
`n`  
The minimum length of storage to be allocated for the container.

### Remarks
If `AutoRehash` is true, this function will also adjust the number of buckets to an appropriate value that can contain at least `n` elements and rebuild the hash table if necessary.

## `destroy_container`
Empty the container and release all resources, including the hash table.
```C++
void destroy_container() noexcept;
```
### Remarks
After calling this function, any attempt to insert, search, or remove elements results in undefined behavior.   
If `AutoRehash` is true, you can call the insert function afterward, and the hash table will be automatically rebuilt.

## `_index_find`
Finds an element that matches a specified key.
```C++
index _index_find(const key_type& keyval)
```
### Parameters
`keyval`  
Key value to search for.

### Return value
An index that refers to the location of an element with a specified key, or `npos` if no match is found for the key.


其餘未詳細說明的函數請參考C++標準。  
For other functions not described in detail, please refer to the C++ standard.
