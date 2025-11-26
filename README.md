# mapped-memory

The goal of MappedObject is to be able to put allocate a lot of data without alloc failure because we run out of RAM. 

It uses boost memory mapped region that will flush to disk some of the data when its not needed anymore but you can still access it when needed by having a ptr to it.

To see the difference you can verify that the test:
- `GIVEN_8000_objects_lot_of_data_in_memory_with_memory_hog`: sig killed 
- `GIVEN_8000_objects_lot_of_data_with_memory_hog`: works fine

```
make && ctest -R GIVEN_8000_objects_lot_of_data_with_memory_hog --verbose
```