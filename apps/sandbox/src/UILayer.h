#pragma once

#include <engine/Layer.h>
#include <engine/ui/Canvas.h>
#include <engine/ui/RmlUiLayer.h>
#include <engine/Application.h>

class UILayer : public se::Layer {
   public:
    UILayer() : Layer("UILayer") {}

    void OnAttach() override {
        // Example 1: Load from external RML file
        canvas_from_file_ = se::Canvas::Create("DemoCanvas");
        canvas_from_file_->LoadFromRML("assets/ui/demo.rml");
        
        // Add event listeners to the buttons from the file
        auto button1 = canvas_from_file_->GetElementById("demo-button");
        if (button1) {
            button1->AddEventListener("click", []() {
                SE_LOG_INFO("File-based Button 1 Clicked!");
            });
            button1->AddEventListener("mouseover", []() {
                SE_LOG_INFO("File-based Button 1 Hovered!");
            });
        }
        
        auto button2 = canvas_from_file_->GetElementById("demo-button-2");
        if (button2) {
            button2->AddEventListener("click", []() {
                SE_LOG_INFO("File-based Button 2 Clicked!");
            });
        }
        
        canvas_from_file_->Show();
        
        // Example 2: Create programmatically (commented out by default to avoid overlap)
        // Uncomment this section to see programmatic UI creation
        /*
        canvas_programmatic_ = se::Canvas::Create("ProgrammaticCanvas");
        
        // Create a div container
        auto container = canvas_programmatic_->CreateElement("div");
        if (container) {
            container->SetProperty("width", "300px");
            container->SetProperty("height", "200px");
            container->SetProperty("margin", "50px auto");
            container->SetProperty("padding", "20px");
            container->SetProperty("background-color", "#34495e");
            container->SetProperty("border", "2px solid #3498db");
        }
        
        // Create a button inside
        auto button = canvas_programmatic_->CreateElement("button");
        if (button) {
            button->SetText("Programmatic Button");
            button->SetProperty("display", "block");
            button->SetProperty("width", "200px");
            button->SetProperty("padding", "15px");
            button->SetProperty("margin", "10px auto");
            button->SetProperty("background-color", "#e74c3c");
            button->SetProperty("color", "#ecf0f1");
            button->SetProperty("text-align", "center");
            button->SetProperty("font-size", "16px");
            button->SetProperty("border", "none");
            button->SetProperty("cursor", "pointer");
            
            button->AddEventListener("click", []() {
                SE_LOG_INFO("Programmatic Button Clicked!");
            });
            
            button->AddEventListener("mouseover", []() {
                SE_LOG_INFO("Programmatic Button Hovered!");
            });
        }
        
        canvas_programmatic_->Show();
        */
    }

    void OnDetach() override {
        if (canvas_from_file_) canvas_from_file_->Hide();
        if (canvas_programmatic_) canvas_programmatic_->Hide();
    }

   private:
    std::shared_ptr<se::Canvas> canvas_from_file_;
    std::shared_ptr<se::Canvas> canvas_programmatic_;
};
