void determineMinMax(
    uint ignore_count,
    global uint *gist
) {
    uint darkCount = 0;

    for (uint i = 0; i < 256; i++) {
        uint darkIndex = i;
        uint element = gist[darkIndex];
        if (darkCount < ignore_count) {
            darkCount += element;
        }

        if (darkCount >= ignore_count && element != 0) {
            gist[1] = darkIndex;
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
            break;
        }
    }
}

kernel void makeGist(
    global uchar *data,
    global uint *gist,
    const int chunk_size,
    const uint ignore_count,
    const uint data_size,
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

    for (uint i = data_from; i < data_to; i++) {
        int local_data_index = i - group_data_from;
        local_data[local_data_index] = data[i];
    }

    local uint local_gist[256];
    for (uint i = 0; i < max(1, int(256 / group_size)); i++) {
        uint index = i * group_size + local_id;
        local_gist[index] = 0;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint i = data_from; i < data_to; i++) {
        int local_data_index = i - group_data_from;
        atomic_inc(local_gist + local_data[local_data_index]);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    for (uint i = 0; i < 256 / group_size; i++) {
        uint index = i * group_size + local_id;
        atomic_add(gist + index, local_gist[index]);
    }

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    uint min_v = 255;
    uint max_v = 0;

    if (global_id == 0) {
        determineMinMax(
            ignore_count,
            gist
        );
    }

    barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    min_v = gist[1];
    max_v = gist[2];

    float const scale = 255 / float(max_v - min_v);
    float const scaledMinV = scale * float(min_v);

    for (uint i = data_from; i < data_to; i++) {
        int local_data_index = i - group_data_from;
        int scaledValue = scale * local_data[local_data_index] - scaledMinV;
        data[i] = max(0, min(scaledValue, 255));
    }
}