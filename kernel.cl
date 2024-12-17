kernel void makeGist(global const uchar *data, global size_t *gist, const int chunk_size)
{
    uint global_id = get_global_id(0);
    uint data_from = global_id * chunk_size;
    uint data_to = (global_id + 1) * chunk_size;
    uint offset = 256 * global_id;
    for (uint i = data_from; i < data_to; i++) {
        size_t index = (data[i] + offset);
        gist[index] += 1;
    }
}

kernel void modify(global uchar *data, const float scale, const float scaledMinValue)
{
    uint i = get_global_id(0);
    int scaledValue = scale * data[i] - scaledMinValue;
    data[i] = max(0, min(scaledValue, 255));
}