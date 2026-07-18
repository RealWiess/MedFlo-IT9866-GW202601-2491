set(nghttp2_dir ${PROJECT_SOURCE_DIR}/sdk/share/nghttp2)

set(nghttp2_files
    ${nghttp2_dir}/lib/nghttp2_pq.c
    ${nghttp2_dir}/lib/nghttp2_map.c
    ${nghttp2_dir}/lib/nghttp2_queue.c
    ${nghttp2_dir}/lib/nghttp2_frame.c
    ${nghttp2_dir}/lib/nghttp2_buf.c
    ${nghttp2_dir}/lib/nghttp2_stream.c 
    ${nghttp2_dir}/lib/nghttp2_outbound_item.c
    ${nghttp2_dir}/lib/nghttp2_session.c
    ${nghttp2_dir}/lib/nghttp2_submit.c
    ${nghttp2_dir}/lib/nghttp2_helper.c
    ${nghttp2_dir}/lib/nghttp2_npn.c
    ${nghttp2_dir}/lib/nghttp2_hd.c
    ${nghttp2_dir}/lib/nghttp2_hd_huffman.c
    ${nghttp2_dir}/lib/nghttp2_hd_huffman_data.c
    ${nghttp2_dir}/lib/nghttp2_version.c
    ${nghttp2_dir}/lib/nghttp2_priority_spec.c
    ${nghttp2_dir}/lib/nghttp2_option.c
    ${nghttp2_dir}/lib/nghttp2_callbacks.c
    ${nghttp2_dir}/lib/nghttp2_mem.c
    ${nghttp2_dir}/lib/nghttp2_http.c
    ${nghttp2_dir}/lib/nghttp2_debug.c
    ${nghttp2_dir}/lib/nghttp2_rcbuf.c
)

set(nghttp2_includes
    ${nghttp2_dir}/lib/nghttp2_pq.h
    ${nghttp2_dir}/lib/nghttp2_map.h
    ${nghttp2_dir}/lib/nghttp2_queue.h
    ${nghttp2_dir}/lib/nghttp2_frame.h
    ${nghttp2_dir}/lib/nghttp2_buf.h
    ${nghttp2_dir}/lib/nghttp2_stream.h 
    ${nghttp2_dir}/lib/nghttp2_outbound_item.h
    ${nghttp2_dir}/lib/nghttp2_session.h
    ${nghttp2_dir}/lib/nghttp2_submit.h
    ${nghttp2_dir}/lib/nghttp2_helper.h
    ${nghttp2_dir}/lib/nghttp2_npn.h
    ${nghttp2_dir}/lib/nghttp2_hd.h
    ${nghttp2_dir}/lib/nghttp2_hd_huffman.h
    ${nghttp2_dir}/lib/nghttp2_priority_spec.h
    ${nghttp2_dir}/lib/nghttp2_option.h
    ${nghttp2_dir}/lib/nghttp2_callbacks.h
    ${nghttp2_dir}/lib/nghttp2_mem.h
    ${nghttp2_dir}/lib/nghttp2_http.h
    ${nghttp2_dir}/lib/nghttp2_debug.h
    ${nghttp2_dir}/lib/nghttp2_int.h
    ${nghttp2_dir}/lib/nghttp2_net.h
    ${nghttp2_dir}/lib/nghttp2_rcbuf.h
)

add_library(nghttp2 STATIC
    ${nghttp2_files}
    ${nghttp2_includes}
)

target_include_directories(nghttp2
PRIVATE
    ${PROJECT_SOURCE_DIR}/openrtos/include/
    ${PROJECT_SOURCE_DIR}/sdk/include/
    ${PROJECT_SOURCE_DIR}/sdk/share/nghttp2/lib/
    ${PROJECT_SOURCE_DIR}/sdk/share/nghttp2/lib/includes
)

target_compile_definitions(nghttp2
PRIVATE
    -DBUILDING_NGHTTP2
    -DHAVE_ARPA_INET_H
)

#set(NGHTTP2_RES "")



