#pragma once

#include <dsk/err.hpp>
#include <dsk/util/stringify.hpp>
#include <libpq-fe.h>


namespace dsk{


enum class [[nodiscard]] pq_errc
{
    connect_failed = 1,
    connect_init_failed,
    connect_reset_failed,
    set_non_blocking_failed,
    send_query_failed,
    send_query_params_failed,
    send_query_prepared_failed,
    send_prepare_failed,
    send_describe_prepared_failed,
    send_describe_portal_failed,
    send_close_prepared_failed,
    send_close_portal_failed,
    send_flush_request_failed,
    send_pipeline_sync_failed,
    send_copy_failed,
    send_copy_buf_full,
    send_copy_end_failed,
    send_copy_end_buf_full,
    enter_pipeline_mode_failed,
    exit_pipeline_mode_failed,
    get_copy_row_failed,
    flush_failed,
    consume_input_failed,
    set_single_row_mode_failed,
    set_chunked_rows_mode_failed,
    result_bad_response,
    result_nonfatal_error,
    result_empty_query,
    result_fatal_error,
    result_unknown_error,
    null_for_non_optional_type,
    unsupported_textual_conv,
    unsupported_binary_conv,
};


class pq_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "postgres"; }

    std::string message(int condition) const override
    {
        switch(static_cast<pq_errc>(condition))
        {
            case pq_errc::connect_failed               : return "connect failed";
            case pq_errc::connect_init_failed          : return "connect init failed";
            case pq_errc::connect_reset_failed         : return "connect reset failed";
            case pq_errc::set_non_blocking_failed      : return "set non blocking failed";
            case pq_errc::send_query_failed            : return "send query failed";
            case pq_errc::send_query_params_failed     : return "send query parameters failed";
            case pq_errc::send_query_prepared_failed   : return "send query prepared failed";
            case pq_errc::send_prepare_failed          : return "send prepare failed";
            case pq_errc::send_describe_prepared_failed: return "send describe prepared failed";
            case pq_errc::send_describe_portal_failed  : return "send describe portal failed";
            case pq_errc::send_close_prepared_failed   : return "send close prepared failed";
            case pq_errc::send_close_portal_failed     : return "send close portal failed";
            case pq_errc::send_flush_request_failed    : return "send flush request failed";
            case pq_errc::send_pipeline_sync_failed    : return "send pipeline sync failed";
            case pq_errc::send_copy_failed             : return "send copy failed";
            case pq_errc::send_copy_buf_full           : return "send copy buffer full";
            case pq_errc::send_copy_end_failed         : return "send copy end failed";
            case pq_errc::send_copy_end_buf_full       : return "send copy end buffer full";
            case pq_errc::enter_pipeline_mode_failed   : return "enter pipeline mode failed";
            case pq_errc::exit_pipeline_mode_failed    : return "exit pipeline mode failed";
            case pq_errc::get_copy_row_failed          : return "get copy row failed";
            case pq_errc::flush_failed                 : return "flush failed";
            case pq_errc::consume_input_failed         : return "consume input failed";
            case pq_errc::set_single_row_mode_failed   : return "set single row mode failed";
            case pq_errc::set_chunked_rows_mode_failed : return "set chunked rows mode failed";
            case pq_errc::result_bad_response          : return "result: bad response";
            case pq_errc::result_nonfatal_error        : return "result: nonfatal error";
            case pq_errc::result_empty_query           : return "result: empty query";
            case pq_errc::result_fatal_error           : return "result: fatal error";
            case pq_errc::result_unknown_error         : return "result: unknown error";
            case pq_errc::null_for_non_optional_type   : return "null for non optional type";
            case pq_errc::unsupported_textual_conv     : return "unsupported textual conversion";
            case pq_errc::unsupported_binary_conv      : return "unsupported binary conversion";
        }

        return "undefined";
    }
};


inline constexpr pq_err_category g_pq_err_cat;


// https://www.postgresql.org/docs/current/errcodes-appendix.html
enum class [[nodiscard]] pq_sql_errc
{
    warning                                       = 46656,
    warning_dynamic_result_sets_returned          = 46668,
    warning_implicit_zero_bit_padding             = 46664,
    warning_null_value_eliminated_in_set_function = 46659,
    warning_privilege_not_granted                 = 46663,
    warning_privilege_not_revoked                 = 46662,
    warning_string_data_right_truncation          = 46660,
    warning_deprecated_feature                    = 79057,
    no_data                                       = 93312,
    no_additional_dynamic_result_sets_returned    = 93313,
    sql_stmt_not_yet_complete                     = 139968,
    connection_exception                          = 373248,
    connection_does_not_exist                     = 373251,
    connection_failure                            = 373254,
    client_unable_to_establish_connection         = 373249,
    server_rejected_establishment_of_connection   = 373252,
    txn_resolution_unknown                        = 373255,
    protocol_violation                            = 405649,
    triggered_action_exception                    = 419904,
    feature_not_supported                         = 466560,
    invalid_txn_initiation                        = 513216,
    locator_exception                             = 699840,
    invalid_locator_specification                 = 699841,
    invalid_grantor                               = 979776,
    invalid_grant_operation                       = 1012177,
    invalid_role_specification                    = 1166400,
    diags_exception                               = 1632960,
    stacked_diags_accessed_without_active_handler = 1632962,
    case_not_found                                = 3359232,
    cardinality_violation                         = 3405888,
    data_exception                                = 3452544,
    array_element_error                           = 3452630,
    character_not_in_repertoire                   = 3452617,
    datetime_field_overflow                       = 3452552,
    division_by_zero                              = 3452582,
    error_in_assignment                           = 3452549,
    escape_character_conflict                     = 3452555,
    indicator_overflow                            = 3452618,
    interval_field_overflow                       = 3452585,
    invalid_argument_for_log                      = 3452594,
    invalid_argument_for_ntile                    = 3452584,
    invalid_argument_for_nth_value                = 3452586,
    invalid_argument_for_power_function           = 3452595,
    invalid_argument_for_width_bucket_function    = 3452596,
    invalid_character_value_for_cast              = 3452588,
    invalid_datetime_format                       = 3452551,
    invalid_escape_character                      = 3452589,
    invalid_escape_octet                          = 3452557,
    invalid_escape_sequence                       = 3452621,
    nonstandard_use_of_escape_character           = 3484950,
    invalid_indicator_parameter_value             = 3452580,
    invalid_parameter_value                       = 3452619,
    invalid_preceding_or_following_size           = 3452583,
    invalid_regular_expression                    = 3452591,
    invalid_row_count_in_limit_clause             = 3452612,
    invalid_row_count_in_result_offset_clause     = 3452613,
    invalid_tablesample_argument                  = 3452633,
    invalid_tablesample_repeat                    = 3452632,
    invalid_time_zone_displacement_value          = 3452553,
    invalid_use_of_escape_character               = 3452556,
    most_specific_type_mismatch                   = 3452560,
    null_value_not_allowed                        = 3452548,
    null_value_no_indicator_parameter             = 3452546,
    numeric_value_out_of_range                    = 3452547,
    sequence_generator_limit_exceeded             = 3452561,
    string_data_length_mismatch                   = 3452622,
    string_data_right_truncation                  = 3452545,
    substring_error                               = 3452581,
    trim_error                                    = 3452623,
    unterminated_c_string                         = 3452620,
    zero_length_character_string                  = 3452559,
    floating_point_exception                      = 3484945,
    invalid_text_representation                   = 3484946,
    invalid_binary_representation                 = 3484947,
    bad_copy_file_format                          = 3484948,
    untranslatable_character                      = 3484949,
    not_an_xml_document                           = 3452565,
    invalid_xml_document                          = 3452566,
    invalid_xml_content                           = 3452567,
    invalid_xml_comment                           = 3452572,
    invalid_xml_processing_instruction            = 3452573,
    duplicate_json_object_key_value               = 3452652,
    invalid_argument_for_json_datetime_function   = 3452653,
    invalid_json_text                             = 3452654,
    invalid_json_subscript                        = 3452655,
    more_than_one_json_item                       = 3452656,
    no_json_item                                  = 3452657,
    non_numeric_json_item                         = 3452658,
    non_unique_keys_in_a_json_object              = 3452659,
    singleton_json_item_required                  = 3452660,
    json_array_not_found                          = 3452661,
    json_member_not_found                         = 3452662,
    json_number_not_found                         = 3452663,
    json_object_not_found                         = 3452664,
    too_many_json_array_elements                  = 3452665,
    too_many_json_object_members                  = 3452666,
    json_scalar_required                          = 3452667,
    json_item_cannot_be_cast_to_target_type       = 3452668,
    integrity_constraint_violation                = 3499200,
    restrict_violation                            = 3499201,
    not_null_violation                            = 3505682,
    foreign_key_violation                         = 3505683,
    unique_violation                              = 3505685,
    check_violation                               = 3505720,
    exclusion_violation                           = 3531601,
    invalid_cursor_state                          = 3545856,
    invalid_txn_state                             = 3592512,
    active_txn                                    = 3592513,
    branch_txn_already_active                     = 3592514,
    held_cursor_requires_same_isolation_level     = 3592520,
    inappropriate_access_mode_for_branch_txn      = 3592515,
    inappropriate_isolation_level_for_branch_txn  = 3592516,
    no_active_txn_for_branch_txn                  = 3592517,
    read_only_txn                                 = 3592518,
    schema_and_data_stmt_mixing_not_supported     = 3592519,
    no_active_txn                                 = 3624913,
    in_failed_txn                                 = 3624914,
    idle_in_txn_session_timeout                   = 3624915,
    invalid_stmt_name                             = 3639168,
    triggered_data_change_violation               = 3685824,
    invalid_auth_specification                    = 3732480,
    invalid_password                              = 3764881,
    dependent_privilege_descriptors_still_exist   = 3872448,
    dependent_objects_still_exist                 = 3904849,
    invalid_txn_termination                       = 3965760,
    sql_routine_exception                         = 4059072,
    sre_function_executed_no_return_stmt          = 4059077,
    sre_modifying_data_not_permitted              = 4059074,
    sre_prohibited_stmt_attempted                 = 4059075,
    sre_reading_data_not_permitted                = 4059076,
    invalid_cursor_name                           = 5225472,
    external_routine_exception                    = 5412096,
    ere_containing_sql_not_permitted              = 5412097,
    ere_modifying_data_not_permitted              = 5412098,
    ere_prohibited_stmt_attempted                 = 5412099,
    ere_reading_data_not_permitted                = 5412100,
    external_routine_invocation_exception         = 5458752,
    erie_invalid_sqlstate_returned                = 5458753,
    erie_null_value_not_allowed                   = 5458756,
    erie_trigger_protocol_violated                = 5491153,
    erie_srf_protocol_violated                    = 5491154,
    erie_event_trigger_protocol_violated          = 5491155,
    savepoint_exception                           = 5552064,
    se_invalid_specification                      = 5552065,
    invalid_catalog_name                          = 5645376,
    invalid_schema_name                           = 5738688,
    txn_rollback                                  = 6718464,
    tr_integrity_constraint_violation             = 6718466,
    tr_serialization_failure                      = 6718465,
    tr_stmt_completion_unknown                    = 6718467,
    tr_deadlock_detected                          = 6750865,
    syntax_error_or_access_rule_violation         = 6811776,
    syntax_error                                  = 6819553,
    insufficient_privilege                        = 6818257,
    cannot_coerce                                 = 6822294,
    grouping_error                                = 6822147,
    windowing_error                               = 6844248,
    invalid_recursion                             = 6844221,
    invalid_foreign_key                           = 6822252,
    invalid_name                                  = 6819554,
    name_too_long                                 = 6819626,
    reserved_name                                 = 6823557,
    datatype_mismatch                             = 6822148,
    indeterminate_datatype                        = 6844220,
    collation_mismatch                            = 6844249,
    indeterminate_collation                       = 6844250,
    wrong_object_type                             = 6822153,
    generated_always                              = 6822585,
    undefined_column                              = 6820851,
    undefined_function                            = 6822435,
    undefined_table                               = 6844177,
    undefined_parameter                           = 6844178,
    undefined_object                              = 6820852,
    duplicate_column                              = 6820849,
    duplicate_cursor                              = 6844179,
    duplicate_database                            = 6844180,
    duplicate_function                            = 6820923,
    duplicate_pstmt                               = 6844181,
    duplicate_schema                              = 6844182,
    duplicate_table                               = 6844183,
    duplicate_alias                               = 6820886,
    duplicate_object                              = 6820884,
    ambiguous_column                              = 6820850,
    ambiguous_function                            = 6820925,
    ambiguous_parameter                           = 6844184,
    ambiguous_alias                               = 6844185,
    invalid_column_ref                            = 6844212,
    invalid_column_definition                     = 6819589,
    invalid_cursor_definition                     = 6844213,
    invalid_database_definition                   = 6844214,
    invalid_function_definition                   = 6844215,
    invalid_pstmt_definition                      = 6844216,
    invalid_schema_definition                     = 6844217,
    invalid_table_definition                      = 6844218,
    invalid_object_definition                     = 6844219,
    with_check_option_violation                   = 6905088,
    insufficient_resources                        = 8538048,
    disk_full                                     = 8539344,
    out_of_memory                                 = 8540640,
    too_many_connections                          = 8541936,
    configuration_limit_exceeded                  = 8543232,
    program_limit_exceeded                        = 8584704,
    stmt_too_complex                              = 8584705,
    too_many_columns                              = 8584741,
    too_many_arguments                            = 8584779,
    object_not_in_prerequisite_state              = 8631360,
    object_in_use                                 = 8631366,
    cant_change_runtime_parameter                 = 8663762,
    lock_not_available                            = 8663763,
    unsafe_new_enum_value_usage                   = 8663764,
    operator_intervention                         = 8724672,
    query_canceled                                = 8724712,
    admin_shutdown                                = 8757073,
    crash_shutdown                                = 8757074,
    cannot_connect_now                            = 8757075,
    database_dropped                              = 8757076,
    idle_session_timeout                          = 8757077,
    system_error                                  = 8771328,
    io_error                                      = 8771436,
    undefined_file                                = 8803729,
    duplicate_file                                = 8803730,
    snapshot_too_old                              = 11850624,
    config_file_error                             = 25194240,
    lock_file_exists                              = 25194241,
    fdw_error                                     = 29999808,
    fdw_column_name_not_found                     = 29999813,
    fdw_dynamic_parameter_value_needed            = 29999810,
    fdw_function_sequence_error                   = 29999844,
    fdw_inconsistent_descriptor_info              = 29999881,
    fdw_invalid_attribute_value                   = 29999884,
    fdw_invalid_column_name                       = 29999815,
    fdw_invalid_column_number                     = 29999816,
    fdw_invalid_data_type                         = 29999812,
    fdw_invalid_data_type_descriptors             = 29999814,
    fdw_invalid_descriptor_field_id               = 30000133,
    fdw_invalid_handle                            = 29999819,
    fdw_invalid_option_index                      = 29999820,
    fdw_invalid_option_name                       = 29999821,
    fdw_invalid_string_or_buf_length              = 30000132,
    fdw_invalid_string_format                     = 29999818,
    fdw_invalid_use_of_null_pointer               = 29999817,
    fdw_too_many_handles                          = 29999848,
    fdw_out_of_memory                             = 29999809,
    fdw_no_schemas                                = 29999833,
    fdw_option_name_not_found                     = 29999827,
    fdw_reply_handle                              = 29999828,
    fdw_schema_not_found                          = 29999834,
    fdw_table_not_found                           = 29999835,
    fdw_unable_to_create_execution                = 29999829,
    fdw_unable_to_create_reply                    = 29999830,
    fdw_unable_to_establish_connection            = 29999831,
    plpgsql_error                                 = 41990400,
    raise_exception                               = 41990401,
    no_data_found                                 = 41990402,
    too_many_rows                                 = 41990403,
    assert_failure                                = 41990404,
    internal_error                                = 56966976,
    data_corrupted                                = 56966977,
    index_corrupted                               = 56966978,
};


class pq_sql_err_category : public error_category
{
public:
    char const* name() const noexcept override { return "postgres sql"; }

    std::string message(int condition) const override
    {
        switch(static_cast<pq_sql_errc>(condition))
        {
            case pq_sql_errc::warning                                      : return "warning";
            case pq_sql_errc::warning_dynamic_result_sets_returned         : return "warning: dynamic result sets returned";
            case pq_sql_errc::warning_implicit_zero_bit_padding            : return "warning: implicit zero bit padding";
            case pq_sql_errc::warning_null_value_eliminated_in_set_function: return "warning: null value eliminated in set function";
            case pq_sql_errc::warning_privilege_not_granted                : return "warning: privilege not granted";
            case pq_sql_errc::warning_privilege_not_revoked                : return "warning: privilege not revoked";
            case pq_sql_errc::warning_string_data_right_truncation         : return "warning: string data right truncation";
            case pq_sql_errc::warning_deprecated_feature                   : return "warning: deprecated feature";
            case pq_sql_errc::no_data                                      : return "no data";
            case pq_sql_errc::no_additional_dynamic_result_sets_returned   : return "no additional dynamic result sets returned";
            case pq_sql_errc::sql_stmt_not_yet_complete                    : return "sql statement not yet complete";
            case pq_sql_errc::connection_exception                         : return "connection exception";
            case pq_sql_errc::connection_does_not_exist                    : return "connection does not exist";
            case pq_sql_errc::connection_failure                           : return "connection failure";
            case pq_sql_errc::client_unable_to_establish_connection        : return "sqlclient unable to establish sqlconnection";
            case pq_sql_errc::server_rejected_establishment_of_connection  : return "sqlserver rejected establishment of sqlconnection";
            case pq_sql_errc::txn_resolution_unknown                       : return "transaction resolution unknown";
            case pq_sql_errc::protocol_violation                           : return "protocol violation";
            case pq_sql_errc::triggered_action_exception                   : return "triggered action exception";
            case pq_sql_errc::feature_not_supported                        : return "feature not supported";
            case pq_sql_errc::invalid_txn_initiation                       : return "invalid transaction initiation";
            case pq_sql_errc::locator_exception                            : return "locator exception";
            case pq_sql_errc::invalid_locator_specification                : return "invalid locator specification";
            case pq_sql_errc::invalid_grantor                              : return "invalid grantor";
            case pq_sql_errc::invalid_grant_operation                      : return "invalid grant operation";
            case pq_sql_errc::invalid_role_specification                   : return "invalid role specification";
            case pq_sql_errc::diags_exception                              : return "diagnostics exception";
            case pq_sql_errc::stacked_diags_accessed_without_active_handler: return "stacked diagnostics accessed without active handler";
            case pq_sql_errc::case_not_found                               : return "case not found";
            case pq_sql_errc::cardinality_violation                        : return "cardinality violation";
            case pq_sql_errc::data_exception                               : return "data exception";
            case pq_sql_errc::array_element_error                          : return "array element error";
            case pq_sql_errc::character_not_in_repertoire                  : return "character not in repertoire";
            case pq_sql_errc::datetime_field_overflow                      : return "datetime field overflow";
            case pq_sql_errc::division_by_zero                             : return "division by zero";
            case pq_sql_errc::error_in_assignment                          : return "error in assignment";
            case pq_sql_errc::escape_character_conflict                    : return "escape character conflict";
            case pq_sql_errc::indicator_overflow                           : return "indicator overflow";
            case pq_sql_errc::interval_field_overflow                      : return "interval field overflow";
            case pq_sql_errc::invalid_argument_for_log                     : return "invalid argument for log";
            case pq_sql_errc::invalid_argument_for_ntile                   : return "invalid argument for ntile";
            case pq_sql_errc::invalid_argument_for_nth_value               : return "invalid argument for nth value";
            case pq_sql_errc::invalid_argument_for_power_function          : return "invalid argument for power function";
            case pq_sql_errc::invalid_argument_for_width_bucket_function   : return "invalid argument for width bucket functiontion";
            case pq_sql_errc::invalid_character_value_for_cast             : return "invalid character value for cast";
            case pq_sql_errc::invalid_datetime_format                      : return "invalid datetime format";
            case pq_sql_errc::invalid_escape_character                     : return "invalid escape character";
            case pq_sql_errc::invalid_escape_octet                         : return "invalid escape octet";
            case pq_sql_errc::invalid_escape_sequence                      : return "invalid escape sequence";
            case pq_sql_errc::nonstandard_use_of_escape_character          : return "nonstandard use of escape character";
            case pq_sql_errc::invalid_indicator_parameter_value            : return "invalid indicator parameter value";
            case pq_sql_errc::invalid_parameter_value                      : return "invalid parameter value";
            case pq_sql_errc::invalid_preceding_or_following_size          : return "invalid preceding or following size";
            case pq_sql_errc::invalid_regular_expression                   : return "invalid regular expression";
            case pq_sql_errc::invalid_row_count_in_limit_clause            : return "invalid row count in limit clause";
            case pq_sql_errc::invalid_row_count_in_result_offset_clause    : return "invalid row count in result offset clause";
            case pq_sql_errc::invalid_tablesample_argument                 : return "invalid tablesample argument";
            case pq_sql_errc::invalid_tablesample_repeat                   : return "invalid tablesample repeat";
            case pq_sql_errc::invalid_time_zone_displacement_value         : return "invalid time zone displacement value";
            case pq_sql_errc::invalid_use_of_escape_character              : return "invalid use of escape character";
            case pq_sql_errc::most_specific_type_mismatch                  : return "most specific type mismatch";
            case pq_sql_errc::null_value_not_allowed                       : return "null value not allowed";
            case pq_sql_errc::null_value_no_indicator_parameter            : return "null value no indicator parameter";
            case pq_sql_errc::numeric_value_out_of_range                   : return "numeric value out of range";
            case pq_sql_errc::sequence_generator_limit_exceeded            : return "sequence generator limit exceeded";
            case pq_sql_errc::string_data_length_mismatch                  : return "string data length mismatch";
            case pq_sql_errc::string_data_right_truncation                 : return "string data right truncation";
            case pq_sql_errc::substring_error                              : return "substring error";
            case pq_sql_errc::trim_error                                   : return "trim error";
            case pq_sql_errc::unterminated_c_string                        : return "unterminated c string";
            case pq_sql_errc::zero_length_character_string                 : return "zero length character string";
            case pq_sql_errc::floating_point_exception                     : return "floating point exception";
            case pq_sql_errc::invalid_text_representation                  : return "invalid text representation";
            case pq_sql_errc::invalid_binary_representation                : return "invalid binary representation";
            case pq_sql_errc::bad_copy_file_format                         : return "bad copy file format";
            case pq_sql_errc::untranslatable_character                     : return "untranslatable character";
            case pq_sql_errc::not_an_xml_document                          : return "not an xml document";
            case pq_sql_errc::invalid_xml_document                         : return "invalid xml document";
            case pq_sql_errc::invalid_xml_content                          : return "invalid xml content";
            case pq_sql_errc::invalid_xml_comment                          : return "invalid xml comment";
            case pq_sql_errc::invalid_xml_processing_instruction           : return "invalid xml processing instruction";
            case pq_sql_errc::duplicate_json_object_key_value              : return "duplicate json object key value";
            case pq_sql_errc::invalid_argument_for_json_datetime_function  : return "invalid argument for json datetime function";
            case pq_sql_errc::invalid_json_text                            : return "invalid json text";
            case pq_sql_errc::invalid_json_subscript                       : return "invalid json subscript";
            case pq_sql_errc::more_than_one_json_item                      : return "more than one json item";
            case pq_sql_errc::no_json_item                                 : return "no json item";
            case pq_sql_errc::non_numeric_json_item                        : return "non numeric json item";
            case pq_sql_errc::non_unique_keys_in_a_json_object             : return "non unique keys in a json object";
            case pq_sql_errc::singleton_json_item_required                 : return "singleton json item required";
            case pq_sql_errc::json_array_not_found                         : return "json array not found";
            case pq_sql_errc::json_member_not_found                        : return "json member not found";
            case pq_sql_errc::json_number_not_found                        : return "json number not found";
            case pq_sql_errc::json_object_not_found                        : return "json object not found";
            case pq_sql_errc::too_many_json_array_elements                 : return "too many json array elements";
            case pq_sql_errc::too_many_json_object_members                 : return "too many json object members";
            case pq_sql_errc::json_scalar_required                         : return "json scalar required";
            case pq_sql_errc::json_item_cannot_be_cast_to_target_type      : return "json item cannot be cast to target type";
            case pq_sql_errc::integrity_constraint_violation               : return "integrity constraint violation";
            case pq_sql_errc::restrict_violation                           : return "restrict violation";
            case pq_sql_errc::not_null_violation                           : return "not null violation";
            case pq_sql_errc::foreign_key_violation                        : return "foreign key violation";
            case pq_sql_errc::unique_violation                             : return "unique violation";
            case pq_sql_errc::check_violation                              : return "check violation";
            case pq_sql_errc::exclusion_violation                          : return "exclusion violation";
            case pq_sql_errc::invalid_cursor_state                         : return "invalid cursor state";
            case pq_sql_errc::invalid_txn_state                            : return "invalid transaction state";
            case pq_sql_errc::active_txn                                   : return "active transaction";
            case pq_sql_errc::branch_txn_already_active                    : return "branch transaction already active";
            case pq_sql_errc::held_cursor_requires_same_isolation_level    : return "held cursor requires same isolation level";
            case pq_sql_errc::inappropriate_access_mode_for_branch_txn     : return "inappropriate access mode for branch transaction";
            case pq_sql_errc::inappropriate_isolation_level_for_branch_txn : return "inappropriate isolation level for branch transaction";
            case pq_sql_errc::no_active_txn_for_branch_txn                 : return "no active transaction for branch transaction";
            case pq_sql_errc::read_only_txn                                : return "read only transaction";
            case pq_sql_errc::schema_and_data_stmt_mixing_not_supported    : return "schema and data statement mixing not supported";
            case pq_sql_errc::no_active_txn                                : return "no active transaction";
            case pq_sql_errc::in_failed_txn                                : return "in failed transaction";
            case pq_sql_errc::idle_in_txn_session_timeout                  : return "idle in transaction session timeout";
            case pq_sql_errc::invalid_stmt_name                            : return "invalid statement name";
            case pq_sql_errc::triggered_data_change_violation              : return "triggered data change violation";
            case pq_sql_errc::invalid_auth_specification                   : return "invalid auth specification";
            case pq_sql_errc::invalid_password                             : return "invalid password";
            case pq_sql_errc::dependent_privilege_descriptors_still_exist  : return "dependent privilege descriptors still exist";
            case pq_sql_errc::dependent_objects_still_exist                : return "dependent objects still exist";
            case pq_sql_errc::invalid_txn_termination                      : return "invalid transaction termination";
            case pq_sql_errc::sql_routine_exception                        : return "sql routine exception (sre)";
            case pq_sql_errc::sre_function_executed_no_return_stmt         : return "sre: function executed no return statement";
            case pq_sql_errc::sre_modifying_data_not_permitted             : return "sre: modifying data not permitted";
            case pq_sql_errc::sre_prohibited_stmt_attempted                : return "sre: prohibited statement attempted";
            case pq_sql_errc::sre_reading_data_not_permitted               : return "sre: reading data not permitted";
            case pq_sql_errc::invalid_cursor_name                          : return "invalid cursor name";
            case pq_sql_errc::external_routine_exception                   : return "external routine exception (ere)";
            case pq_sql_errc::ere_containing_sql_not_permitted             : return "ere: containing sql not permitted";
            case pq_sql_errc::ere_modifying_data_not_permitted             : return "ere: modifying data not permitted";
            case pq_sql_errc::ere_prohibited_stmt_attempted                : return "ere: prohibited statement attempted";
            case pq_sql_errc::ere_reading_data_not_permitted               : return "ere: reading data not permitted";
            case pq_sql_errc::external_routine_invocation_exception        : return "external routine invocation exception (erie)";
            case pq_sql_errc::erie_invalid_sqlstate_returned               : return "erie:e invalid sqlstate returned";
            case pq_sql_errc::erie_null_value_not_allowed                  : return "erie:e null value not allowed";
            case pq_sql_errc::erie_trigger_protocol_violated               : return "erie:e trigger protocol violated";
            case pq_sql_errc::erie_srf_protocol_violated                   : return "erie:e srf protocol violated";
            case pq_sql_errc::erie_event_trigger_protocol_violated         : return "erie:e event trigger protocol violated";
            case pq_sql_errc::savepoint_exception                          : return "savepoint exception (se)";
            case pq_sql_errc::se_invalid_specification                     : return "se: invalid specification";
            case pq_sql_errc::invalid_catalog_name                         : return "invalid catalog name";
            case pq_sql_errc::invalid_schema_name                          : return "invalid schema name";
            case pq_sql_errc::txn_rollback                                 : return "transaction rollback (tr)";
            case pq_sql_errc::tr_integrity_constraint_violation            : return "tr: integrity constraint violation";
            case pq_sql_errc::tr_serialization_failure                     : return "tr: serialization failure";
            case pq_sql_errc::tr_stmt_completion_unknown                   : return "tr: statement completion unknown";
            case pq_sql_errc::tr_deadlock_detected                         : return "tr: deadlock detected";
            case pq_sql_errc::syntax_error_or_access_rule_violation        : return "syntax error or access rule violation";
            case pq_sql_errc::syntax_error                                 : return "syntax error";
            case pq_sql_errc::insufficient_privilege                       : return "insufficient privilege";
            case pq_sql_errc::cannot_coerce                                : return "cannot coerce";
            case pq_sql_errc::grouping_error                               : return "grouping error";
            case pq_sql_errc::windowing_error                              : return "windowing error";
            case pq_sql_errc::invalid_recursion                            : return "invalid recursion";
            case pq_sql_errc::invalid_foreign_key                          : return "invalid foreign key";
            case pq_sql_errc::invalid_name                                 : return "invalid name";
            case pq_sql_errc::name_too_long                                : return "name too long";
            case pq_sql_errc::reserved_name                                : return "reserved name";
            case pq_sql_errc::datatype_mismatch                            : return "datatype mismatch";
            case pq_sql_errc::indeterminate_datatype                       : return "indeterminate datatype";
            case pq_sql_errc::collation_mismatch                           : return "collation mismatch";
            case pq_sql_errc::indeterminate_collation                      : return "indeterminate collation";
            case pq_sql_errc::wrong_object_type                            : return "wrong object type";
            case pq_sql_errc::generated_always                             : return "generated always";
            case pq_sql_errc::undefined_column                             : return "undefined column";
            case pq_sql_errc::undefined_function                           : return "undefined function";
            case pq_sql_errc::undefined_table                              : return "undefined table";
            case pq_sql_errc::undefined_parameter                          : return "undefined parameter";
            case pq_sql_errc::undefined_object                             : return "undefined object";
            case pq_sql_errc::duplicate_column                             : return "duplicate column";
            case pq_sql_errc::duplicate_cursor                             : return "duplicate cursor";
            case pq_sql_errc::duplicate_database                           : return "duplicate database";
            case pq_sql_errc::duplicate_function                           : return "duplicate function";
            case pq_sql_errc::duplicate_pstmt                              : return "duplicate prepared statement";
            case pq_sql_errc::duplicate_schema                             : return "duplicate schema";
            case pq_sql_errc::duplicate_table                              : return "duplicate table";
            case pq_sql_errc::duplicate_alias                              : return "duplicate alias";
            case pq_sql_errc::duplicate_object                             : return "duplicate object";
            case pq_sql_errc::ambiguous_column                             : return "ambiguous column";
            case pq_sql_errc::ambiguous_function                           : return "ambiguous function";
            case pq_sql_errc::ambiguous_parameter                          : return "ambiguous parameter";
            case pq_sql_errc::ambiguous_alias                              : return "ambiguous alias";
            case pq_sql_errc::invalid_column_ref                           : return "invalid column ref";
            case pq_sql_errc::invalid_column_definition                    : return "invalid column definition";
            case pq_sql_errc::invalid_cursor_definition                    : return "invalid cursor definition";
            case pq_sql_errc::invalid_database_definition                  : return "invalid database definition";
            case pq_sql_errc::invalid_function_definition                  : return "invalid function definition";
            case pq_sql_errc::invalid_pstmt_definition                     : return "invalid prepared statement definition";
            case pq_sql_errc::invalid_schema_definition                    : return "invalid schema definition";
            case pq_sql_errc::invalid_table_definition                     : return "invalid table definition";
            case pq_sql_errc::invalid_object_definition                    : return "invalid object definition";
            case pq_sql_errc::with_check_option_violation                  : return "with check option violation";
            case pq_sql_errc::insufficient_resources                       : return "insufficient resources";
            case pq_sql_errc::disk_full                                    : return "disk full";
            case pq_sql_errc::out_of_memory                                : return "out of memory";
            case pq_sql_errc::too_many_connections                         : return "too many connections";
            case pq_sql_errc::configuration_limit_exceeded                 : return "configuration limit exceeded";
            case pq_sql_errc::program_limit_exceeded                       : return "program limit exceeded";
            case pq_sql_errc::stmt_too_complex                             : return "statement too complex";
            case pq_sql_errc::too_many_columns                             : return "too many columns";
            case pq_sql_errc::too_many_arguments                           : return "too many arguments";
            case pq_sql_errc::object_not_in_prerequisite_state             : return "object not in prerequisite state";
            case pq_sql_errc::object_in_use                                : return "object in use";
            case pq_sql_errc::cant_change_runtime_parameter                : return "cant change runtime parameter";
            case pq_sql_errc::lock_not_available                           : return "lock not available";
            case pq_sql_errc::unsafe_new_enum_value_usage                  : return "unsafe new enum value usage";
            case pq_sql_errc::operator_intervention                        : return "operator intervention";
            case pq_sql_errc::query_canceled                               : return "query canceled";
            case pq_sql_errc::admin_shutdown                               : return "admin shutdown";
            case pq_sql_errc::crash_shutdown                               : return "crash shutdown";
            case pq_sql_errc::cannot_connect_now                           : return "cannot connect now";
            case pq_sql_errc::database_dropped                             : return "database dropped";
            case pq_sql_errc::idle_session_timeout                         : return "idle session timeout";
            case pq_sql_errc::system_error                                 : return "system error";
            case pq_sql_errc::io_error                                     : return "io error";
            case pq_sql_errc::undefined_file                               : return "undefined file";
            case pq_sql_errc::duplicate_file                               : return "duplicate file";
            case pq_sql_errc::snapshot_too_old                             : return "snapshot too old";
            case pq_sql_errc::config_file_error                            : return "config file error";
            case pq_sql_errc::lock_file_exists                             : return "lock file exists";
            case pq_sql_errc::fdw_error                                    : return "foreign data wrapper (fdw) error";
            case pq_sql_errc::fdw_column_name_not_found                    : return "fdw: column name not found";
            case pq_sql_errc::fdw_dynamic_parameter_value_needed           : return "fdw: dynamic parameter value needed";
            case pq_sql_errc::fdw_function_sequence_error                  : return "fdw: function sequence error";
            case pq_sql_errc::fdw_inconsistent_descriptor_info             : return "fdw: inconsistent descriptor info";
            case pq_sql_errc::fdw_invalid_attribute_value                  : return "fdw: invalid attribute value";
            case pq_sql_errc::fdw_invalid_column_name                      : return "fdw: invalid column name";
            case pq_sql_errc::fdw_invalid_column_number                    : return "fdw: invalid column number";
            case pq_sql_errc::fdw_invalid_data_type                        : return "fdw: invalid data type";
            case pq_sql_errc::fdw_invalid_data_type_descriptors            : return "fdw: invalid data type descriptors";
            case pq_sql_errc::fdw_invalid_descriptor_field_id              : return "fdw: invalid descriptor field id";
            case pq_sql_errc::fdw_invalid_handle                           : return "fdw: invalid handle";
            case pq_sql_errc::fdw_invalid_option_index                     : return "fdw: invalid option index";
            case pq_sql_errc::fdw_invalid_option_name                      : return "fdw: invalid option name";
            case pq_sql_errc::fdw_invalid_string_or_buf_length             : return "fdw: invalid string or buf length";
            case pq_sql_errc::fdw_invalid_string_format                    : return "fdw: invalid string format";
            case pq_sql_errc::fdw_invalid_use_of_null_pointer              : return "fdw: invalid use of null pointer";
            case pq_sql_errc::fdw_too_many_handles                         : return "fdw: too many handles";
            case pq_sql_errc::fdw_out_of_memory                            : return "fdw: out of memory";
            case pq_sql_errc::fdw_no_schemas                               : return "fdw: no schemas";
            case pq_sql_errc::fdw_option_name_not_found                    : return "fdw: option name not found";
            case pq_sql_errc::fdw_reply_handle                             : return "fdw: reply handle";
            case pq_sql_errc::fdw_schema_not_found                         : return "fdw: schema not found";
            case pq_sql_errc::fdw_table_not_found                          : return "fdw: table not found";
            case pq_sql_errc::fdw_unable_to_create_execution               : return "fdw: unable to create execution";
            case pq_sql_errc::fdw_unable_to_create_reply                   : return "fdw: unable to create reply";
            case pq_sql_errc::fdw_unable_to_establish_connection           : return "fdw: unable to establish connection";
            case pq_sql_errc::plpgsql_error                                : return "plpgsql error";
            case pq_sql_errc::raise_exception                              : return "raise exception";
            case pq_sql_errc::no_data_found                                : return "no data found";
            case pq_sql_errc::too_many_rows                                : return "too many rows";
            case pq_sql_errc::assert_failure                               : return "assert failure";
            case pq_sql_errc::internal_error                               : return "internal error";
            case pq_sql_errc::data_corrupted                               : return "data corrupted";
            case pq_sql_errc::index_corrupted                              : return "index corrupted";
        }

        return cat_as_str<std::string>("undefined: ", with_radix<36>(condition));
    }
};


inline constexpr pq_sql_err_category g_pq_sql_err_cat;


} // namespace dsk


DSK_REGISTER_ERROR_CODE_ENUM(dsk, pq_errc, g_pq_err_cat)
DSK_REGISTER_ERROR_CODE_ENUM(dsk, pq_sql_errc, g_pq_sql_err_cat)
