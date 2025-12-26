#pragma once

struct SampleEventWithNoInputs {};
struct SampleEventWithOneInput {
    int input;
    SampleEventWithOneInput(int i) : input(i) {}
};
struct SampleEventWithTwoInputs {
    int input_1;
    int input_2;
    SampleEventWithTwoInputs(int i1, int i2) : input_1(i1), input_2(i2) {}
};