#define VEC_REALLOC 10
#define BIG_ENOUGH 200
#define MULTIPLIER 1.5
#include <cstdlib>

template <typename T>
struct vector{
private:
    T * ptr;
    int size;
    int capacity;
public:
    //vector(int sz=0, int cp=0)
    //    :ptr(new T[cp]), size(sz), capacity(cp)
    //{}
    vector(int sz=0)
        :ptr((T *)malloc(sz*sizeof(T))), size(sz), capacity(sz)
    {}
    vector(int sz, int cp)
        :ptr((T *)malloc(cp*sizeof(T))), size(sz), capacity(cp)
    {}
    void push_back(T a){
        if(size<capacity){
            ptr[size++]=a;
        }
        else{
            reallocate();
            ptr[size++]=a;
        }
    }
    // void reallocate(int n=0){
    //     int delta = is_big_enough()? capacity*MULTIPLIER:VEC_REALLOC;
    //     int new_capacity = capacity+(n>delta?n:delta);

    //     T* new_ptr = new T[new_capacity];
    //     for(int i=0;i<size;i++){
    //         new_ptr[i] = ptr[i];
    //     }
    //     delete [] ptr;
    //     ptr = new_ptr;
    //     capacity = new_capacity;
    // }
    void reallocate(int n=0){
        int delta = is_big_enough()? capacity*MULTIPLIER:VEC_REALLOC;
        int new_capacity = capacity+(n>delta?n:delta);
        
        ptr = (T *)realloc(ptr, new_capacity*sizeof(T));
        capacity = new_capacity;
    }
    T operator[](int i){
        return *(ptr+i);
    }
    int is_big_enough(){
        return capacity>BIG_ENOUGH?1:0;
    }
    int getsize(){
        return size;
    }
    T* getptr(){
        return ptr;
    }
    ~vector(){
        printf("his size and capacity were %d, %d\n", size, capacity);
        free(ptr);
    }

};
