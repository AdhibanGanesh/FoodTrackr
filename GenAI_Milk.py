from flask import Flask, request, jsonify
import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

app = Flask(__name__)

# Load model and tokenizer
model_name = "microsoft/phi-2"
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name,
                                             torch_dtype=torch.float16,
                                             device_map="auto")

# Set device
device = "cuda" if torch.cuda.is_available() else "cpu"
model.to(device)

# Generate summary from sensor data
def generate_text(prompt, max_length=150):
    inputs = tokenizer(prompt, return_tensors="pt").to(device)
    outputs = model.generate(**inputs, max_length=max_length)
    return tokenizer.decode(outputs[0], skip_special_tokens=True)

@app.route('/summarize', methods=['POST'])
def summarize():
    try:
        data = request.json
        sensor_data = data.get("sensor_data", "")
        if not sensor_data:
            return jsonify({"error": "Missing sensor_data"}), 400

        summary_prompt = f"Summarize the milk condition based on sensor readings:\n{sensor_data}\n\nSummary:"
        summary_report = generate_text(summary_prompt)

        summary_start = summary_report.find("Summary:")
        summary = summary_report[summary_start:].strip() if summary_start != -1 else summary_report.strip()

        return jsonify({"summary": summary})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
