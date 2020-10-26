import tensorflow as tf

tf1 = tf.compat.v1

# 42 Total lands // 7 features
MAP_Y = 7
MAP_X = 6
MAP_SIZE = MAP_X * MAP_Y

PLAYER_FEATURES = 3

# IVT 1
#FEATURES = PLAYER_FEATURES + 9
#FILE_POSTFIX = 'V1'

# IVT2
FEATURES = PLAYER_FEATURES + 10
FILE_POSTFIX = 'V2'

# IVT3
# FEATURES = PLAYER_FEATURES + 11
# FILE_POSTFIX = 'V3'

INPUT_SIZE = MAP_SIZE * FEATURES

LANDS = 42
OUTPUT_POLICY_SIZE = LANDS + 1
OUTPUT_VALUE_SIZE = 1

REGULARIZATION_L2_C = 0.001
LEARNING_RATE = 0.001
FILTERS = 256

BLOCKS = 20


def residual_block(inputLayer, filters, kernel_size, stage, block, training, l2_reg):
        conv_name = 'res' + str(stage) + block + '_branch'
        bn_name = 'bn' + str(stage) + block + '_branch'

        shortcut = inputLayer

        residual_layer = tf1.layers.conv2d(inputLayer, filters, kernel_size=(kernel_size, kernel_size), strides=(1, 1), name=conv_name + '2a', padding='same', use_bias=False, kernel_regularizer=l2_reg)
        residual_layer = tf1.layers.batch_normalization(residual_layer, axis=3, name=bn_name + '2a', training=training)
        residual_layer = tf1.nn.relu(residual_layer)
        residual_layer = tf1.layers.conv2d(residual_layer, filters, kernel_size=(kernel_size, kernel_size), strides=(1, 1), name=conv_name + '2b', padding='same', use_bias=False, kernel_regularizer=l2_reg)
        residual_layer = tf1.layers.batch_normalization(residual_layer, axis=3, name=bn_name + '2b', training=training)
        add_shortcut = tf1.add(residual_layer, shortcut)
        residual_result = tf1.nn.relu(add_shortcut)

        return residual_result


def build_graph(export):
    tf1.disable_eager_execution()

    graph = tf1.Graph()
    with graph.as_default():
        with tf1.device('/gpu:0'):
            l2_reg = tf.keras.regularizers.l2(l=REGULARIZATION_L2_C)
            # l2_reg = None

            input_boards = tf1.placeholder(tf.float32, shape=[None, MAP_Y, MAP_X, FEATURES], name="input_state")
            is_training = tf1.placeholder(tf.bool, name="input_training")

            # x_image = tf1.reshape(input_boards, [-1, MAP_Y, MAP_X, FEATURES]) # [batch_size, board_y, x board_x, features]
            x_image = tf1.layers.conv2d(input_boards, FILTERS, kernel_size=(3, 3), strides=(1, 1), name='conv', padding='same', use_bias=False, kernel_regularizer=l2_reg)
            x_image = tf1.layers.batch_normalization(x_image, axis=1, name='conv_bn', training=is_training)
            x_image = tf1.nn.relu(x_image)

            residual_tower = residual_block(inputLayer=x_image, kernel_size=3, filters=FILTERS, stage=0, block='a', training=is_training, l2_reg=l2_reg)

            for i in range(1, BLOCKS):
                residual_tower = residual_block(inputLayer=residual_tower, kernel_size=3, filters=FILTERS, stage=i, block=chr(ord('a') + i), training=is_training, l2_reg=l2_reg)

            policy = tf1.layers.conv2d(residual_tower, 2, kernel_size=(1, 1), strides=(1, 1), name='pi', padding='same', use_bias=False, kernel_regularizer=l2_reg)
            policy = tf1.layers.batch_normalization(policy, axis=3, name='bn_pi', training=is_training)
            policy = tf1.nn.relu(policy)
            policy = tf1.layers.flatten(policy, name='p_flatten')
            pi = tf1.layers.dense(policy, OUTPUT_POLICY_SIZE, kernel_regularizer=l2_reg)
            prob = tf1.nn.softmax(pi, name="output_policy")

            value = tf1.layers.conv2d(residual_tower, 1, kernel_size=(1, 1), strides=(1, 1), name='v', padding='same', use_bias=False, kernel_regularizer=l2_reg)
            value = tf1.layers.batch_normalization(value, axis=3, name='bn_v', training=is_training)
            value = tf1.nn.relu(value)
            value = tf1.layers.flatten(value, name='v_flatten')
            value = tf1.layers.dense(value, units=256, kernel_regularizer=l2_reg)
            value = tf1.nn.relu(value)
            value = tf1.layers.dense(value, 1, kernel_regularizer=l2_reg)
            v = tf1.nn.tanh(value, name="output_value")

            target_pis = tf1.placeholder(tf.float32, shape=[None, OUTPUT_POLICY_SIZE], name="target_policy")
            target_vs = tf1.placeholder(tf.float32, shape=[None, 1], name="target_value")
            loss_pi = tf1.losses.softmax_cross_entropy(target_pis, pi)
            loss_v = tf1.losses.mean_squared_error(target_vs, tf.reshape(v, shape=[-1, 1]))

            l2_loss = tf1.losses.get_regularization_loss()
            total_loss = loss_pi + loss_v + l2_loss

            update_ops = tf1.get_collection(tf1.GraphKeys.UPDATE_OPS)

            with tf.control_dependencies(update_ops):
                train_op = tf1.train.AdamOptimizer(LEARNING_RATE, name="optimize").minimize(total_loss)

        init = tf1.global_variables_initializer()
        saver_def = tf1.train.Saver().as_saver_def()

        if export:
            print('Input State Placeholder                        : ', input_boards.name)
            print('Output Policy                                  : ', prob.name)
            print('Input Policy Target                            : ', target_pis.name)
            print('Output Value                                   : ', v.name)
            print('Input Value Target                             : ', target_vs.name)
            print('Run this operation to initialize variables     : ', init.name)
            print('Run this operation for a train step            : ', train_op.name)
            print('Output value loss for policy                   : ', loss_pi.name)
            print('Output value loss for value                    : ', loss_v.name)
            print('Feed this tensor to set the checkpoint filename: ', saver_def.filename_tensor_name)
            print('Run this operation to save a checkpoint        : ', saver_def.save_tensor_name)
            print('Run this operation to restore a checkpoint     : ', saver_def.restore_op_name)

            tf1.train.write_graph(graph.as_graph_def(), 'model', 'model_bin_%s_%d.pb' % (FILE_POSTFIX, BLOCKS), as_text=False)
            tf1.train.write_graph(graph.as_graph_def(), 'model', 'model_txt_%s_%d.pb' % (FILE_POSTFIX, BLOCKS), as_text=True)

    return graph


if __name__ == "__main__":
    build_graph(True)








