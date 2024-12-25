kernel void determineMinMax(
    uint ignore_count,
    global uint *gist
) {
    uint darkCount = 0;

    for (uint i = 0; i < 256; i++) {
        uint darkIndex = i;
        int element = gist[darkIndex];
        if (darkCount < ignore_count) {
            darkCount += element;
            //gist[255-i] = element;
        }

        if (darkCount >= ignore_count && element != 0) {
            gist[1] = darkIndex;
            //gist[3] = darkCount;
            break;
        }
    }

    uint brightCount = 0;

    for (uint i = 255; i >= 0; i--) {
        uint brightIndex = i;
        uint element = gist[brightIndex];
        if (brightCount < ignore_count) {
            brightCount += element;
        }

        if (brightCount >= ignore_count && element != 0) {
            gist[2] = brightIndex;
            //gist[4] = brightCount;
            break;
        }
    }
}

kernel void makeGist(
    global uchar *data,
    global uint *gist,
    const int chunk_size,
    const uint ignore_count,
    const uint data_size
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
    после расчётов собираем глобальную гистограмму
    определяем min max одним ворк айтемом
    скейлим значения из глобальной памяти

    */

    uint global_id = get_global_id(0);
    uint local_id = get_local_id(0);
    uint group_id = get_group_id(0);
    uint group_size = get_local_size(0);
    uint groups_count = get_global_size(0) / group_size;

    uint group_data_from = group_id * chunk_size;
    uint group_data_to = min(data_size, (group_id + 1) * chunk_size);

    uint item_chunk_size = (chunk_size / group_size) + 1;
    uint data_from = group_data_from + item_chunk_size * local_id;
    uint data_to = min(group_data_to, data_from + item_chunk_size);

    // for (uint i = data_from; i < data_to; i++) {
    //     int local_data_index = i - group_data_from;
    //     local_data[local_data_index] = data[i];
    // }

    local uint local_gist[256];
    uint max_index = 256 / group_size;
    bool isGistHandler = (max_index < 1 && local_id < 256) || max_index >= 1;
    uint min_max_index = 1;
    max_index = max(min_max_index, max_index);
    uint max_handled_group_size = 256;
    if (isGistHandler) {
        for (uint i = 0; i < max_index; i++) {
            uint index = i * min(group_size, max_handled_group_size) + local_id;
            local_gist[index] = 0;
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint i = data_from; i < data_to; i++) {
        // int local_data_index = i - group_data_from;
        atomic_inc(local_gist + data[i]);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (isGistHandler) {
        for (uint i = 0; i < max_index; i++) {
            uint index = i * min(group_size, max_handled_group_size) + local_id;
            atomic_add(gist + index, local_gist[index]);
        }
    }
}

kernel void modify(global uchar *data, const float scale, const float scaledMinValue)
{
    uint i = get_global_id(0);
    int scaledValue = scale * data[i] - scaledMinValue;
    data[i] = max(0, min(scaledValue, 255));
}