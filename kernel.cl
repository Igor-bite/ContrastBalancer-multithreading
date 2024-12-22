kernel void makeGist(
    global const uchar *data,
    global uint *gist,
    const int chunk_size,
    local uchar *local_data
) {
    /*

    разделяем данные на чанки
    1 чанк на 1 группу
    кол-во чанков = data_size / groups_count
    данные картинки передаём через глобальную память
    копируем свой чанк в локальную память силами воркайтемов в группе
    создаём локальную гистограмму для группы
    разделяем данные в группе на чанки
    кол-во чанков - chunk_size / group_work-items_count
    в каждом воркайтеме проходимся по своему чанку
    атомарно делаем += 1 в локальную гистограмму
       после расчётов собираем глобальную гистограмму - ВОПРОС КАК?
       определяем min max - ВОПРОС КАК?
    скейлим значения из глобальной памяти

    */

    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);

    uint data_from = local_id * chunk_size;
    uint data_to = (local_id + 1) * chunk_size;

    for (uint i = data_from; i < data_to; i++) {
        local_data[i] = data[i];
    }

    for (uint i = data_from; i < data_to; i++) {
        atomic_inc(gist + local_data[i]);
    }

    // work with local mem
    // use atomics
    // sync before exporting to global mem

//    for (uint i = data_from; i < data_to; i++) {
//        size_t index = (data[i] + offset) * 2;
//        gist[index] += 1;
//    }
}

kernel void modify(global uchar *data, const float scale, const float scaledMinValue)
{
    uint i = get_global_id(0);
    int scaledValue = scale * data[i] - scaledMinValue;
    data[i] = max(0, min(scaledValue, 255));
}