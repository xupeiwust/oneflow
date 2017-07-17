#include "oneflow/core/operator/boxing_op.h"

namespace oneflow {

TEST(BoxingOp, box_4_10x5x6x6) {
  // input shape is
  // in1 {10, 5, 6, 6}
  // in2 {10, 4, 6, 6}
  // in3 {10, 4, 6, 6}
  // in3 {10, 4, 6, 6}
  OperatorConf op_conf;
  op_conf.set_name("boxing_test");
  BoxingOpConf* boxing_conf = op_conf.mutable_boxing_conf();
  boxing_conf->set_lbn("boxing_blob");
  boxing_conf->set_in_num(4);
  boxing_conf->set_out_num(3);
  std::vector<int64_t> input_shape_vec1 = {10, 5, 6, 6};
  std::vector<int64_t> input_shape_vec2 = {10, 4, 6, 6};

  // test concat_box shape function
  boxing_conf->mutable_concat_box()->set_axis(1);
  boxing_conf->mutable_data_split_box();
  auto boxing_op = ConstructOp(op_conf);
  HashMap<std::string, Shape*> bn2shape_ptr{
      {boxing_op->input_bns()[0], new Shape(input_shape_vec2)},
      {boxing_op->input_bns()[1], new Shape(input_shape_vec2)},
      {boxing_op->input_bns()[2], new Shape(input_shape_vec2)},
      {boxing_op->input_bns()[3], new Shape(input_shape_vec1)},
      {"middle", new Shape},
      {boxing_op->output_bns()[0], new Shape},
      {boxing_op->output_bns()[1], new Shape},
      {boxing_op->output_bns()[2], new Shape},
  };
  auto fp = [&bn2shape_ptr](const std::string& bn) {
    return bn2shape_ptr.at(bn);
  };

  // do infer shape
  boxing_op->InferShape4FwBlobs(fp, kModelParallel, 0, 1);

  // test results
  // output_shape should be:
  // out1 {4, 17, 6, 6}
  // out2 {3, 17, 6, 6}
  // out3 {3, 17, 6, 6}
  for (size_t i = 0; i < boxing_op->output_bns().size(); ++i) {
    Shape* output_shape_ptr = bn2shape_ptr.at(boxing_op->output_bns()[i]);
    std::vector<int64_t> output_shape_vec = {3, 17, 6, 6};
    if (i == 0) { output_shape_vec[0] = 4; }
    ASSERT_EQ(*output_shape_ptr, Shape(output_shape_vec));
  }

  // Test add clone box shape function
  boxing_conf->set_in_num(3);
  boxing_conf->set_out_num(1);
  boxing_conf->mutable_add_box();
  boxing_conf->mutable_clone_box();
  boxing_op = ConstructOp(op_conf);

  // do infer shape
  boxing_op->InferShape4FwBlobs(fp, kModelParallel, 0, 1);

  // test results
  // output shape should be the same as input
  for (const std::string& bn : boxing_op->output_bns()) {
    Shape* output_shape_ptr = bn2shape_ptr.at(bn);
    ASSERT_EQ(*output_shape_ptr, Shape(input_shape_vec2));
  }

  // Test concat clone shape function, this box has data_tmp_shape
  boxing_conf->set_in_num(4);
  boxing_conf->set_out_num(1);
  boxing_conf->mutable_concat_box()->set_axis(1);
  boxing_conf->mutable_clone_box();
  boxing_op = ConstructOp(op_conf);

  // do infer shape
  boxing_op->InferShape4FwBlobs(fp, kModelParallel, 0, 1);

  // data_tmp_shape is {10, 17, 6, 6}, and the 17 = 4 + 4 + 4 + 5
  Shape* data_tmp_shape_ptr = bn2shape_ptr.at(boxing_op->SoleDtbn());
  std::vector<int64_t> data_temp_shape_vec = {10, 17, 6, 6};
  ASSERT_EQ(*data_tmp_shape_ptr, Shape(data_temp_shape_vec));

  // test results
  // output shape should be the same as data_tmp_shape
  data_tmp_shape_ptr = bn2shape_ptr.at(boxing_op->SoleDtbn());
  for (const std::string& bn : boxing_op->output_bns()) {
    Shape* output_shape_ptr = bn2shape_ptr.at(bn);
    ASSERT_EQ(*output_shape_ptr, *data_tmp_shape_ptr);
  }
}

}  // namespace oneflow
