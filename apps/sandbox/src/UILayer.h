#pragma once

#include <engine/Layer.h>
#include <engine/ui/Canvas.h>
#include <engine/ui/RmlUiLayer.h>
#include <engine/Application.h>

class UILayer : public se::Layer {
   public:
    UILayer() : Layer("UILayer") {}

    void OnAttach() override {
        // Create a canvas
        canvas_ = se::Canvas::Create("MainCanvas");
        
        // Create a simple button
        auto button = canvas_->CreateElement("button");
        if (button) {
            button->SetText("Click Me!");
            button->SetProperty("font-family", "Lato");
            button->SetProperty("font-size", "20px");
            button->SetProperty("color", "white");
            button->SetProperty("background-color", "#ddddddff");
            button->SetProperty("padding", "10px");
            button->SetProperty("display", "block");
            button->SetProperty("width", "100px");
            button->SetProperty("text-align", "center");
            button->SetProperty("margin", "auto");
            
            button->AddEventListener("click", []() {
                SE_LOG_INFO("Button Clicked!");
            });

            button->AddEventListener("hover", []() {
                SE_LOG_INFO("Button Hovered!");
            });
        }
        
        canvas_->Show();
    }

    void OnDetach() override {
        canvas_->Hide();
    }

   private:
    std::shared_ptr<se::Canvas> canvas_;
};
