#include<stdio.h>
#include<omp.h>
#include<time.h>
#include<stdlib.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
// Structure to hold data
struct Data {
    int id;
    int age;
    float weight;
    float height; // Additional attribute
    std::string name;
    std::string address;
    std::string email;
    std::string phone;
    std::string city;
    std::string country;
    int salary;
    int years_of_experience; // Additional attribute
    bool employed; // Additional attribute
    float rating; // Additional attribute
    float score; // Additional attribute
};
int Partition(std::vector<Data>& data, int start, int end)   //划分数据
{
    Data temp = data[start];   //以第一个元素为基准
    while (start < end) {
        while (start < end && data[end].score >= temp.score)end--;   //找到第一个比基准小的数
        data[start] = data[end];
        while (start < end && data[start].score <= temp.score)start++;    //找到第一个比基准大的数
        data[end] = data[start];
    }
    data[start] = temp;   //以基准作为分界线
    return start;
}

void quickSort(std::vector<Data>& data, int start, int end)  //并行快排
{
    if (start < end) {
        int pos = Partition(data, start, end);
        #pragma omp parallel sections    //设置并行区域
        {
            //#pragma omp task          //该区域对前部分数据进行排序
            quickSort(data, start, pos - 1);
            //#pragma omp task          //该区域对后部分数据进行排序
            quickSort(data, pos + 1, end);
        }
    }
}


// Compare function for std::sort by score
bool compare_score(const Data & a, const Data & b) {
    return a.score < b.score;
}

// Validation function to check if the array is sorted correctly by score
bool validate_score(const std::vector<Data>& array) {
    for (size_t i = 1; i < array.size(); i++) {
        if (array[i].score < array[i - 1].score)
            return false; // Array is not sorted correctly
    }
    return true; // Array is sorted correctly
}
int main(int argc, char* argv[])
{
     if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <array_size>" << std::endl;
        return 1;
    }

    uint64_t array_size = std::strtoull(argv[1], nullptr, 10);
    if (array_size == 0) {
        std::cout << "Array size must be a positive integer" << std::endl;
        return 1;
    }

    //int n = atoi(argv[2]), i;   //线程数
    //int size = atoi(argv[1]);   //数据大小
    //int* num = (int*)malloc(sizeof(int) * size);

    //double starttime = omp_get_wtime();
//     srand(time(NULL) + rand());   //生成随机数组
//     for (i = 0; i < size; i++)
//         num[i] = rand();
    
   
    std::vector<Data> array(array_size);
    std::vector<Data> arraystd(array_size);
    
    // Initialize data
    srand(29); // Seed the random number generator
    for (uint64_t i = 0; i < array_size; i++) {
        array[i].id = i + 1;
        array[i].age = rand() % 100 + 1; // Random age between 1 and 100
        array[i].weight = static_cast<float>(rand()) / RAND_MAX * 100.0f; // Random weight between 0 and 100
        array[i].height = static_cast<float>(rand()) / RAND_MAX * 200.0f; // Random height between 0 and 200
        array[i].name = "Name" + std::to_string(i);
        array[i].address = "Address" + std::to_string(i);
        array[i].email = "Email" + std::to_string(i) + "@example.com";
        array[i].phone = "+1234567890" + std::to_string(i);
        array[i].city = "City" + std::to_string(i);
        array[i].country = "Country" + std::to_string(i);
        array[i].years_of_experience = rand() % 30; // Random years of experience between 0 and 29
        array[i].employed = rand() % 2; // Random employed status (0 or 1)
        array[i].rating = static_cast<float>(rand()) / RAND_MAX * 5.0f; // Random rating between 0 and 5
        array[i].score = static_cast<float>(rand()) / RAND_MAX * 11000000.0f - 1000000.0f; // Random score between -1000000 and 10000000
    }
    std::copy(array.begin(), array.end(), arraystd.begin());
    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();
    //std::cout << "start_time: " << start_time << " seconds" << std::endl;
    // Sort the array using std::sort by score
    //std::sort(array.begin(), array.end(), compare_score);
    //omp_set_num_threads(8);   //设置线程数
    quickSort(array, 0, array.size()-1);   //并行快排
    // End timer
    auto end_time = std::chrono::high_resolution_clock::now();
    
    //std::cout << "end_time: " << end_time << " seconds" << std::endl;
    double time_spent = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();


    // Print time taken
    std::cout << "Time taken: " << time_spent << " seconds" << std::endl;
    if (validate_score(array))
        std::cout << "Validation: Array is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Array is not sorted correctly by score" << std::endl;
    //// Print the top ten results
    //std::cout << "Top ten results:" << std::endl;
    //for (int i = 0; i < std::min(10, static_cast<int>(array.size())); ++i) {
    //    std::cout << "ID: " << array[i].id << ", Age: " << array[i].age << ", Weight: " << array[i].weight 
    //              << ", Score: " << array[i].score << std::endl;
    //    // Print other attributes as needed
    //}

    //// Print the last ten results
    //std::cout << "Last ten results:" << std::endl;
    //int start_index = std::max(0, static_cast<int>(array.size()) - 10);
    //for (int i = start_index; i < array.size(); ++i) {
    //    std::cout << "ID: " << array[i].id << ", Age: " << array[i].age << ", Weight: " << array[i].weight 
    //              << ", Score: " << array[i].score << std::endl;
    //    // Print other attributes as needed
    //}
//std in order
    //copy origin array
    auto start_time_std = std::chrono::high_resolution_clock::now();
    std::sort(arraystd.begin(), arraystd.end(), compare_score);
    auto end_time_std = std::chrono::high_resolution_clock::now();
    double time_spent_std = std::chrono::duration_cast<std::chrono::duration<double>>(end_time_std - start_time_std).count();
    std::cout << "std Time taken: " << time_spent_std << " seconds" << std::endl;

    // Validate if the array is sorted correctly by score
    
     // Clean up
    if (validate_score(arraystd))
        std::cout << "Validation: Arraystd is sorted correctly by score" << std::endl;
    else
        std::cout << "Validation: Arraystd is not sorted correctly by score" << std::endl;
    return 0;
}
