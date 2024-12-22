void determineMinMax() {

}

kernel void makeGist(global const uchar *data, global size_t *gist, const int chunk_size)
{
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
    uint data_from = global_id * chunk_size;
    uint data_to = (global_id + 1) * chunk_size;
    uint offset = 256 * global_id;

    // work with local mem
    // use atomics
    // sync before exporting to global mem

    for (uint i = data_from; i < data_to; i++) {
        size_t index = (data[i] + offset) * 2;
        gist[index] += 1;
    }
}

kernel void modify(global uchar *data, const float scale, const float scaledMinValue)
{
    uint i = get_global_id(0);
    int scaledValue = scale * data[i] - scaledMinValue;
    data[i] = max(0, min(scaledValue, 255));
}