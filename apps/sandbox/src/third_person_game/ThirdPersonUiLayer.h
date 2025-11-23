#pragma once

#include <engine/Application.h>
#include <engine/Layer.h>
#include <engine/ui/Canvas.h>
#include <engine/ui/RmlUiLayer.h>

class ThirdPersonUiLayer : public se::Layer {
   public:
    ThirdPersonUiLayer() : Layer("UILayer") {}

    void OnAttach() override {
        // Example 1: Load from external RML file
        canvas_from_file_ = se::Canvas::Create("DemoCanvas");
        canvas_from_file_->LoadFromRML("assets/ui/third_person_game/third_person_game.rml");

        // // Add event listeners to the buttons from the file
        // auto button1 = canvas_from_file_->GetElementById("demo-button");
        // if (button1) {
        //     button1->AddEventListener("click", []() { SE_LOG_INFO("File-based Button 1 Clicked!"); });
        //     button1->AddEventListener("mouseover", []() { SE_LOG_INFO("File-based Button 1 Hovered!"); });
        // }

        // auto button2 = canvas_from_file_->GetElementById("demo-button-2");
        // if (button2) {
        //     button2->AddEventListener("click", []() { SE_LOG_INFO("File-based Button 2 Clicked!"); });
        // }

        canvas_from_file_->Show();
    }

    void OnDetach() override {
        if (canvas_from_file_) canvas_from_file_->Hide();
        if (canvas_programmatic_) canvas_programmatic_->Hide();
    }

   private:
    std::shared_ptr<se::Canvas> canvas_from_file_;
    std::shared_ptr<se::Canvas> canvas_programmatic_;
};
